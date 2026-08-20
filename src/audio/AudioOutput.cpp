#include "AudioOutput.h"
namespace auralis
{
void AudioOutput::connect(Plan& plan,const MixerState& mixer,const MidiProject& project,bool includeMidi)
{
    std::vector<int> pending;
    for(const auto& c:mixer.all())pending.push_back(c.id);
    // Model rejects cycles. Kahn ordering ensures every source renders before its destination.
    while(!pending.empty())
    {
        const auto it=std::find_if(pending.begin(),pending.end(),[&](int id)
        {return std::none_of(mixer.sends().begin(),mixer.sends().end(),[&](const auto& r){return r.destination==id&&std::find(pending.begin(),pending.end(),r.source)!=pending.end();});});
        if(it==pending.end())break;
        const auto& c=*mixer.find(*it);Plan::Node node;node.id=c.id;
        node.gain=std::pow(10.0,c.gain/20)*(c.polarity?-1:1);node.pan=c.pan/100;node.width=1-c.stereo/100;node.muted=c.muted;
        const bool anySolo=std::any_of(mixer.all().begin(),mixer.all().end(),[](const auto& s){return s.solo;});
        if(anySolo&&c.id!=0)
        {
            // Keep both upstream sources and downstream returns of a soloed channel audible.
            auto reaches=[&](int from,int target)
            {
                std::array<bool,devices::maximumChannels> visited{};
                std::array<int,devices::maximumChannels> stack{};int count=1;stack[0]=from;visited[from]=true;
                while(count>0)
                {
                    const int id=stack[--count];if(id==target)return true;
                    for(const auto& r:mixer.sends())if(r.source==id&&!visited[r.destination])
                    {visited[r.destination]=true;stack[count++]=r.destination;}
                }
                return false;
            };
            bool connected=false;for(const auto& s:mixer.all())if(s.solo&&(reaches(c.id,s.id)||reaches(s.id,c.id))){connected=true;break;}
            node.muted=node.muted||!connected;
        }
        for(const auto& d:c.devices)
        {
            auto processor=processors[d.id].lock();
            if(!processor){processor=std::make_shared<DeviceProcessor>(d.kind);processors[d.id]=processor;}
            node.chain.push_back({d,std::move(processor),d});
        }
        node.automation=c.automation;
        plan.nodes.push_back(std::move(node));pending.erase(it);
    }
    auto index=[&](int id){for(size_t i=0;i<plan.nodes.size();++i)if(plan.nodes[i].id==id)return static_cast<int>(i);return -1;};
    for(const auto& r:mixer.sends()){const int s=index(r.source),d=index(r.destination);if(s>=0&&d>=0)plan.nodes[s].routes.emplace_back(d,r.amount);}
    if(includeMidi)for(const auto& clip:project.clips)
    {
        const int node=index(clip.track);if(node<0)continue;
        for(const auto& note:MidiProject::renderedNotes(clip))
        {
            if(note.muted)continue;
            if(plan.notes.size()>=editing::maximumNotes)break;
            const double seconds=60.0/(plan.tempo*editing::ppq);
            const int key=static_cast<int>(plan.notes.size());
            plan.notes.push_back({(clip.start+note.start)*seconds,(clip.start+note.start+note.length)*seconds,node,key,note.pitch,note.velocity/127.0,note.pan/100.0,note.finePitch/100.0});
            plan.events.push_back({plan.notes.back().start,key,true});plan.events.push_back({plan.notes.back().end,key,false});
        }
    }
    std::sort(plan.events.begin(),plan.events.end(),[](const auto& a,const auto& b){return a.time==b.time?a.on<b.on:a.time<b.time;});
    std::erase_if(processors,[](const auto& p){return p.second.expired();});
}
std::pair<double,double> AudioOutput::meter(int id) const
{
    if(id<0||id>=devices::maximumChannels)return {};
    return {peaksL[id].load(std::memory_order_relaxed),peaksR[id].load(std::memory_order_relaxed)};
}
AudioOutput::~AudioOutput()
{
    device.removeAudioCallback(this);device.closeAudioDevice();published=nullptr;
}
juce::String AudioOutput::open()
{
    if(ready.load())return {};
    device.removeAudioCallback(this);
    const auto error=device.initialiseWithDefaultDevices(0,2);
    if(error.isNotEmpty())return error;
    device.addAudioCallback(this);return {};
}
void AudioOutput::publish(Plan next)
{
    if(next.clips.size()>audio::maximumVoices)next.clips.resize(audio::maximumVoices);
    auto replacement=std::make_unique<Plan>(std::move(next));published.store(replacement.get());
    if(current)retired.push_back(std::move(current));current=std::move(replacement);collect();
}
void AudioOutput::collect()
{
    const auto* inUse=hazard.load();std::erase_if(retired,[inUse](const auto& plan){return plan.get()!=inUse;});
}
void AudioOutput::audioDeviceAboutToStart(juce::AudioIODevice* d)
{
    sampleRate=d->getCurrentSampleRate();reportedRate=sampleRate;seenSeek=~0u;
    if(const auto* plan=published.load())for(const auto& node:plan->nodes)for(const auto& plugin:node.chain)if(plugin.state.hosted)plugin.state.hosted->prepare(sampleRate);
    ready=true;
}
void AudioOutput::audioDeviceIOCallbackWithContext(const float* const*,int,float* const* output,int channels,int count,const juce::AudioIODeviceCallbackContext&)
{
    struct ActiveGuard {std::atomic<bool>& active;explicit ActiveGuard(std::atomic<bool>& a):active(a){active.store(true);}~ActiveGuard(){active.store(false);}} guard(callbackActive);
    juce::ScopedNoDenormals noDenormals;
    for(int channel=0;channel<channels;++channel)if(output[channel])std::fill_n(output[channel],count,0.0f);
    if(suspended.load())return;
    const auto* plan=published.load();hazard.store(plan);
    if(plan!=published.load()||plan==nullptr){hazard=nullptr;return;}
    const bool seek=plan->seek!=seenSeek;
    if(seek){cursor=plan->start;seenSeek=plan->seek;finished=false;outputRamp=0;tailStopped=false;}
    const double secondsPerTick=60.0/(plan->tempo*editing::ppq);
    const bool resetNotes=seek||seenRevision!=plan->noteRevision;
    const auto previousHeld=held;const int previousTrack=heldTrack;
    if(resetNotes)for(const auto& node:plan->nodes)for(const auto& d:node.chain)
    {d.processor->reset();if(d.state.hosted)d.state.hosted->reset();}
    std::array<double,audio::maximumVoices> speeds{},gains{},starts{},durations{};
    std::array<int,audio::maximumVoices> clipNodes{};clipNodes.fill(-1);
    int master=-1;
    for(size_t n=0;n<plan->nodes.size();++n)
    {
        const auto& node=plan->nodes[n];if(node.id==0)master=static_cast<int>(n);
        if(!channelSeen[node.id]){channelGains[node.id]=node.muted?0:node.gain;channelSeen[node.id]=true;}
    }
    for(size_t i=0;i<plan->clips.size();++i)
    {
        const auto& clip=plan->clips[i];speeds[i]=clip.speed();gains[i]=std::pow(10.0,clip.gainDb/20);
        starts[i]=clip.start*secondsPerTick;durations[i]=clip.length*secondsPerTick;clipNodes[i]=master;
        for(size_t n=0;n<plan->nodes.size();++n)if(plan->nodes[n].id==clip.track)clipNodes[i]=static_cast<int>(n);
        if(priorIds[i]!=clip.id){priorIds[i]=clip.id;priorGains[i]=gains[i];}
    }
    std::array<double,devices::maximumChannels> blockL{},blockR{};
    for(int offset=0;offset<count;)
    {
        const bool playing=plan->playing&&!(plan->audition&&finished.load());
        bool wrapped=false;
        if(playing&&plan->loop&&plan->loopEnd>plan->loopStart&&cursor>=plan->loopEnd)
        {
            cursor=plan->loopStart+std::fmod(cursor-plan->loopStart,plan->loopEnd-plan->loopStart);wrapped=true;
            for(const auto& node:plan->nodes)for(const auto& d:node.chain){d.processor->releaseAll();if(d.state.hosted)d.state.hosted->reset();}
        }
        const bool sourcePlaying=playing&&(plan->renderEnd<0||cursor<plan->renderEnd);
        const bool stopVoices=playing&&!sourcePlaying&&!tailStopped;
        if(stopVoices){for(const auto& node:plan->nodes)for(const auto& d:node.chain)d.processor->releaseAll();tailStopped=true;}
        int chunk=std::min(64,count-offset);
        if(sourcePlaying&&plan->renderEnd>=0)chunk=std::min(chunk,std::max(1,static_cast<int>(std::ceil((plan->renderEnd-cursor)*sampleRate))));
        if(playing&&plan->loop&&plan->loopEnd>plan->loopStart)
            chunk=std::min(chunk,std::max(1,static_cast<int>(std::ceil((plan->loopEnd-cursor)*sampleRate))));
        if(playing&&plan->audition&&!plan->clips.empty())chunk=std::min(chunk,std::max(1,static_cast<int>(std::ceil((durations[0]-cursor)*sampleRate))));
        for(const auto& node:plan->nodes){std::fill_n(node.left.data(),chunk,0.0f);std::fill_n(node.right.data(),chunk,0.0f);}
        std::array<float,64> directL{},directR{};
        if(sourcePlaying)for(size_t i=0;i<plan->clips.size();++i)for(int sample=0;sample<chunk;++sample)
        {
            const double elapsed=cursor+sample/sampleRate-starts[i];
            const double gain=priorGains[i]+(gains[i]-priorGains[i])*static_cast<double>(offset+sample+1)/count;
            const float left=audioSamplePrepared(plan->clips[i],elapsed,durations[i],0,speeds[i],gain),right=audioSamplePrepared(plan->clips[i],elapsed,durations[i],1,speeds[i],gain);
            if(clipNodes[i]>=0){plan->nodes[clipNodes[i]].left[sample]+=left;plan->nodes[clipNodes[i]].right[sample]+=right;}
            else{directL[sample]+=left;directR[sample]+=right;}
        }
        for(size_t n=0;n<plan->nodes.size();++n)
        {
            const auto& node=plan->nodes[n];size_t midiCount=0;
            auto midiEvent=[&](int at,int pitch,double velocity,bool on)
            {if(midiCount<hostEvents->size())(*hostEvents)[midiCount++]={at,pitch,static_cast<float>(velocity),on};};
            if(stopVoices)for(int pitch=0;pitch<128;++pitch)midiEvent(0,pitch,0,false);
            const bool seed=wrapped||(offset==0&&resetNotes);
            if(offset==0||wrapped)
            {
                for(int pitch=0;pitch<128;++pitch)
                {
                    if(!seed&&previousHeld[pitch]&&node.id==previousTrack&&(!plan->held[pitch]||previousTrack!=plan->liveTrack))
                    {midiEvent(0,pitch,0,false);for(const auto& d:node.chain)if(isInstrument(d.state.kind))d.processor->noteOff(100000+pitch);}
                    if(plan->held[pitch]&&node.id==plan->liveTrack&&(seed||!previousHeld[pitch]||previousTrack!=plan->liveTrack))
                    {midiEvent(0,pitch,.65,true);for(const auto& d:node.chain)if(isInstrument(d.state.kind))d.processor->noteOn(100000+pitch,pitch,.65);}
                }
                if(seed&&sourcePlaying)for(const auto& note:plan->notes)if(note.node==static_cast<int>(n)&&note.start<cursor&&note.end>cursor)
                {midiEvent(0,note.pitch,note.velocity,true);for(const auto& d:node.chain)if(isInstrument(d.state.kind))d.processor->noteOn(note.key,note.pitch,note.velocity,note.pan,note.fine);}
            }
            auto begin=std::lower_bound(plan->events.begin(),plan->events.end(),cursor,[](const auto& event,double time){return event.time<time;});
            if(sourcePlaying)for(auto event=begin;event!=plan->events.end()&&event->time<cursor+chunk/sampleRate;++event)
            {const auto& note=plan->notes[event->note];if(note.node==static_cast<int>(n))midiEvent(std::clamp(static_cast<int>((event->time-cursor)*sampleRate),0,chunk-1),note.pitch,note.velocity,event->on);}
            auto automated=[&](int device,int parameter,double fallback,double tick)
            {for(const auto& lane:node.automation)if(lane.enabled&&lane.device==device&&lane.parameter==parameter&&!lane.points.empty())return lane.valueAt(tick);return fallback;};
            for(const auto& d:node.chain)
            {
                if(d.state.kind==DeviceKind::external)
                {
                    if(d.state.hosted&&!d.state.bypass)
                    {
                        for(size_t p=0;p<d.state.externalValues.size();++p)d.state.hosted->parameter(static_cast<int>(p),automated(d.state.id,static_cast<int>(p),d.state.externalValues[p],cursor/secondsPerTick));
                        d.state.hosted->process(node.left.data(),node.right.data(),chunk,{hostEvents->data(),midiCount},plan->tempo,cursor,playing);
                    }
                    for(int sample=0;sample<chunk;++sample)d.processor->analysis.push(node.left[sample],node.right[sample],0,0,sampleRate);
                    continue;
                }
                auto event=begin;
                for(int sample=0;sample<chunk;++sample)
                {
                    const double time=cursor+(playing?sample/sampleRate:0),tick=time/secondsPerTick;
                    if(isInstrument(d.state.kind)&&sourcePlaying)while(event!=plan->events.end()&&event->time<time+1/sampleRate)
                    {
                        const auto& note=plan->notes[event->note];if(note.node==static_cast<int>(n))
                        {if(event->on)d.processor->noteOn(note.key,note.pitch,note.velocity,note.pan,note.fine);else d.processor->noteOff(note.key);}
                        ++event;
                    }
                    for(const auto& lane:node.automation)if(lane.enabled&&lane.device==d.state.id&&lane.parameter>=0&&lane.parameter<devices::maximumParameters&&!lane.points.empty())d.renderState.values[lane.parameter]=lane.valueAt(tick);
                    if(sample%32==0)d.processor->prepare(d.renderState,sampleRate,plan->tempo);
                    const auto value=d.processor->process({node.left[sample],node.right[sample]},d.renderState,sampleRate);
                    node.left[sample]=static_cast<float>(value.left);node.right[sample]=static_cast<float>(value.right);
                }
            }
            const bool automatedGain=std::any_of(node.automation.begin(),node.automation.end(),[](const auto& lane){return lane.enabled&&lane.device==-1&&lane.parameter==0&&!lane.points.empty();});
            for(int sample=0;sample<chunk;++sample)
            {
                const double tick=(cursor+(playing?sample/sampleRate:0))/secondsPerTick;
                const double width=1-automated(-1,2,(1-node.width)*100,tick)/100,pan=automated(-1,1,node.pan*100,tick)/100;
                const double mid=(node.left[sample]+node.right[sample])*.5,side=(node.left[sample]-node.right[sample])*.5*width;
                const double target=node.muted?0:std::copysign(std::pow(10.0,automated(-1,0,20*std::log10(std::max(1e-12,std::abs(node.gain))),tick)/20),node.gain);
                const double gain=automatedGain?target:channelGains[node.id]+(target-channelGains[node.id])*static_cast<double>(offset+sample+1)/count;
                double left=(mid+side)*gain*(pan>0?1-pan:1),right=(mid-side)*gain*(pan<0?1+pan:1);
                if(!std::isfinite(left)||!std::isfinite(right)){left=0;right=0;}
                blockL[node.id]=std::max(blockL[node.id],std::abs(left));blockR[node.id]=std::max(blockR[node.id],std::abs(right));
                for(const auto& route:node.routes)
                {const double send=automated(-1,100+plan->nodes[route.first].id,route.second,tick);plan->nodes[route.first].left[sample]+=static_cast<float>(left*send);plan->nodes[route.first].right[sample]+=static_cast<float>(right*send);}
                if(static_cast<int>(n)==master){directL[sample]=static_cast<float>(left);directR[sample]=static_cast<float>(right);}
            }
        }
        for(int sample=0;sample<chunk;++sample)
        {
            outputRamp=std::min(1.0,outputRamp+1.0/(sampleRate*.005));
            const double left=std::clamp(directL[sample]*plan->leftGain*outputRamp,-1.0,1.0),right=std::clamp(directR[sample]*plan->rightGain*outputRamp,-1.0,1.0);
            if(channels>0&&output[0])output[0][offset+sample]=static_cast<float>(left);
            if(channels>1&&output[1])output[1][offset+sample]=static_cast<float>(right);
            masterAnalysis.push(left,right,0,0,sampleRate);
        }
        if(playing)cursor+=chunk/sampleRate;
        if(plan->audition&&!plan->clips.empty()&&cursor>=durations[0])finished=true;
        offset+=chunk;
    }
    held=plan->held;heldTrack=plan->liveTrack;seenRevision=plan->noteRevision;
    const double decay=std::exp(-count/(sampleRate*.3));
    for(int id=0;id<devices::maximumChannels;++id){peaksL[id].store(std::max(blockL[id],peaksL[id].load(std::memory_order_relaxed)*decay),std::memory_order_relaxed);peaksR[id].store(std::max(blockR[id],peaksR[id].load(std::memory_order_relaxed)*decay),std::memory_order_relaxed);}
    priorGains=gains;reportedPosition=cursor;
    for(const auto& node:plan->nodes)channelGains[node.id]=node.muted?0:node.gain;
    hazard=nullptr;
}
}


