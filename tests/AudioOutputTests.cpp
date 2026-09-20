#include "audio/AudioOutput.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
namespace auralis
{
struct AudioOutputTestAccess
{
    static void render(AudioOutput& output,float** channels,int frames)
    {
        output.sampleRate=48000;
        output.audioDeviceIOCallbackWithContext(nullptr,0,channels,2,frames,{});
    }
};
}
using namespace auralis;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main()
{
    try
    {
        AudioOutput output;
        auto media=std::make_shared<AudioData>();media->sampleRate=48000;
        media->left.assign(48000,0.1f);media->right.assign(48000,-0.1f);
        AudioClip clip;clip.id=1;clip.source=media;clip.sourceEnd=1;clip.length=1920;
        AudioOutput::Plan plan;plan.clips={clip};plan.playing=true;plan.seek=1;
        std::array<float,512> left{},right{};float* channels[]={left.data(),right.data()};
        output.publish(plan);AudioOutputTestAccess::render(output,channels,512);
        require(std::abs(output.position()-512.0/48000)<1e-10,"Playback clock uses rendered sample count");
        require(left.back()>0.09f&&right.back()<-0.09f,"Renderer produces real stereo signal after startup ramp");
        plan.playing=false;plan.seek=2;plan.start=output.position();output.publish(plan);
        AudioOutputTestAccess::render(output,channels,127);
        require(left[0]==0&&right[126]==0,"Pause clears output for variable block length");
        require(std::abs(output.position()-plan.start)<1e-10,"Pause preserves position");
        plan.playing=true;plan.loop=true;plan.loopStart=0.005;plan.loopEnd=0.010;plan.seek=3;plan.start=0;
        output.publish(plan);AudioOutputTestAccess::render(output,channels,512);
        require(output.position()>=0.005&&output.position()<0.010,"Sample-clock loop preserves overshoot");
        AudioOutputTestAccess::render(output,channels,0);
        plan.loop=false;plan.start=0;plan.seek=4;plan.clips.assign(audio::maximumVoices,clip);output.publish(plan);
        double maximumMs=0;
        for(int i=0;i<100;++i)
        {
            const auto start=std::chrono::steady_clock::now();AudioOutputTestAccess::render(output,channels,512);
            maximumMs=std::max(maximumMs,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
            for(float value:left)require(std::isfinite(value)&&std::abs(value)<=1,"Bounded finite mixed output");
        }
        std::atomic<bool> stop{false};
        std::thread callback([&]{while(!stop.load())AudioOutputTestAccess::render(output,channels,64);});
        for(int i=0;i<500;++i){plan.seek++;plan.clips.resize(i%2?1:audio::maximumVoices);output.publish(plan);}
        stop=true;callback.join();output.collect();
        MixerState mixer;MidiProject document;
        const int send=mixer.addSend(),track=mixer.addTrack(false);
        require(track>send&&mixer.find(track)->type=="AUDIO","New audio track remains distinct from interleaved send IDs");
        mixer.toggleRoute(track,0);mixer.toggleRoute(track,send);mixer.sendAmount(track,send,.5);
        clip.track=track;clip.gainDb=0;document.audioClips={clip};
        AudioOutput::Plan routed;routed.clips=document.audioClips;routed.playing=true;routed.seek=1000;
        output.connect(routed,mixer,document);output.publish(routed);AudioOutputTestAccess::render(output,channels,512);
        require(std::abs(left.back()-.05)<1e-5,"Track audio reaches Master exclusively through a half-level send");
        require(output.meter(track).first>.09&&output.meter(send).first>.049&&output.meter(0).first>.049,"Track, return and Master carry measured peaks");
        mixer.toggleMute(send);routed={};routed.clips=document.audioClips;routed.playing=true;routed.seek=1001;output.connect(routed,mixer,document);output.publish(routed);AudioOutputTestAccess::render(output,channels,512);
        require(std::abs(left.back())<1e-6,"Muting the return silences its routed audio");
        mixer.toggleMute(send);mixer.toggleMute(0);
        routed={};clip.track=0;routed.clips={clip};routed.playing=true;routed.seek=1002;output.connect(routed,mixer,document,false);output.publish(routed);AudioOutputTestAccess::render(output,channels,512);
        require(std::abs(left.back())<1e-6,"Browser audition injected into Master obeys Master mute");
        mixer.toggleMute(0);require(mixer.addDevice(1,DeviceKind::synth),"Create synth lazily");
        const int midiId=document.create(1,0);document.addNote(*document.clip(midiId),0,69,960);
        routed={};routed.playing=true;routed.seek=1003;output.connect(routed,mixer,document);output.publish(routed);
        double energy=0;for(int i=0;i<10;++i){AudioOutputTestAccess::render(output,channels,512);for(float sample:left)energy+=sample*sample;}
        require(energy>.01,"Arrangement MIDI creates an audible instrument signal");
        routed={};routed.seek=1004;routed.liveTrack=1;routed.held[60]=true;output.connect(routed,mixer,document,false);output.publish(routed);AudioOutputTestAccess::render(output,channels,512);
        require(output.meter(1).first>.001,"Keyboard audition plays through the linked channel while transport is stopped");
        routed.held.fill(false);output.publish(routed);
        for(int i=0;i<60;++i)AudioOutputTestAccess::render(output,channels,512);
        require(std::all_of(left.begin(),left.end(),[](float v){return std::abs(v)<1e-7;}),"Keyboard note-off completes envelope release without a stuck voice");
        const auto boost=FilterCoefficients::make(1000,6,.707,0,48000);
        require(std::abs(20*std::log10(boost.magnitude(1000,48000))-6)<1e-6,"EQ bell gain at centre matches requested dB");
        DeviceState plain(DeviceKind::synth),modulated=plain;
        modulated.modulation.push_back({1,24,1}); // ENV 2 lengthens ENV 1 attack.
        DeviceProcessor plainSynth(DeviceKind::synth),modSynth(DeviceKind::synth);
        plainSynth.noteOn(1,60,.8);modSynth.noteOn(1,60,.8);
        double plainEnergy=0,modEnergy=0;
        for(int sample=0;sample<4800;++sample)
        {
            const auto a=plainSynth.process({},plain,48000),b=modSynth.process({},modulated,48000);
            plainEnergy+=a.left*a.left;modEnergy+=b.left*b.left;
        }
        require(modEnergy<plainEnergy*.5,"Modulation of an envelope control changes the actual rendered attack");
        plain.modulation={{3,7,.4}};modulated=plain;modulated.modulation.push_back({0,45,1});
        plainSynth.reset();modSynth.reset();plainSynth.noteOn(1,60,.8);modSynth.noteOn(1,60,.8);
        double modulationDifference=0;
        for(int sample=0;sample<4800;++sample)
        {const auto a=plainSynth.process({},plain,48000),b=modSynth.process({},modulated,48000);modulationDifference+=std::abs(a.left-b.left);}
        require(modulationDifference>.01,"Modulation of LFO rate affects downstream oscillator modulation");
        for(int k=2;k<10;++k)
        {
            DeviceState settings(static_cast<DeviceKind>(k));DeviceProcessor processor(settings.kind);processor.prepare(settings,48000);
            double energyOut=0;
            for(int i=0;i<24000;++i)
            {
                const double signal=i<100?std::sin(i*.15)*.1:0;
                const auto sample=processor.process({signal,-signal},settings,48000);
                require(std::isfinite(sample.left)&&std::isfinite(sample.right),"Every built-in effect returns finite samples");energyOut+=sample.left*sample.left;
            }
            require(energyOut>.001,"Effect processing retains signal energy");
        }
        MixerState automatedMixer;MidiProject emptyDocument;
        automatedMixer.automationMode=true;automatedMixer.gain(4,-60);automatedMixer.automationTime=960;automatedMixer.gain(4,0);
        AudioOutput automatedOutput;AudioOutput::Plan automationPlan;clip.track=4;automationPlan.clips={clip};automationPlan.playing=true;automationPlan.seek=1;
        automatedOutput.connect(automationPlan,automatedMixer,emptyDocument);automatedOutput.publish(automationPlan);
        double early=0,late=0;
        for(int block=0;block<47;++block)
        {AudioOutputTestAccess::render(automatedOutput,channels,512);if(block==1)early=std::abs(left.back());if(block==46)late=std::abs(left.back());}
        require(late>early*100&&late>.095,"Automation plays a volume curve on the audio sample clock");
        automationPlan.renderEnd=.1;automationPlan.seek=2;automatedOutput.publish(automationPlan);
        for(int block=0;block<12;++block)AudioOutputTestAccess::render(automatedOutput,channels,512);
        require(std::all_of(left.begin(),left.end(),[](float value){return value==0;}),"Export end excludes later source audio while allowing effect tails");
        std::cout<<"Audio callback clock/loop/pause/variable blocks and 500 concurrent plan swaps passed.\n";
        std::cout<<"128 voices, 48 kHz, 512 frames, 100 blocks: max render "<<maximumMs<<" ms; block duration 10.667 ms (offline, not hardware latency).\n";
        return 0;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
