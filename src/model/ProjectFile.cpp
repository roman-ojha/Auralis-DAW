#include "ProjectFile.h"
#include "constants/Project.h"
#include <set>
#include <stdexcept>

namespace auralis
{
namespace
{
constexpr auto formatVersion=project::formatVersion;
constexpr auto maximumMetadata=project::maximumMetadataBytes;
constexpr auto maximumMediaBytes=project::maximumMediaBytes;
using Xml=juce::XmlElement;
void require(bool valid,const char* reason) { if(!valid)throw std::runtime_error(reason); }
double number(const Xml& xml,const char* name,double low,double high)
{
    require(xml.hasAttribute(name),"Project is missing a numeric field.");
    const auto value=xml.getDoubleAttribute(name,std::numeric_limits<double>::quiet_NaN());
    require(std::isfinite(value)&&value>=low&&value<=high,"Project contains an invalid numeric value.");
    return value;
}
int integer(const Xml& xml,const char* name,int low,int high)
{
    const auto value=number(xml,name,low,high);require(value==std::floor(value),"Project contains a fractional identifier.");return static_cast<int>(value);
}
Tick tick(const Xml& xml,const char* name,Tick minimum=0)
{return static_cast<Tick>(number(xml,name,static_cast<double>(minimum),static_cast<double>(editing::maximumTime)));}
void setTick(Xml& xml,const char* name,Tick value){xml.setAttribute(name,juce::String(value));}
bool flag(const Xml& xml,const char* name){return integer(xml,name,0,1)!=0;}
std::string name(const Xml& xml,const char* attribute)
{const auto value=xml.getStringAttribute(attribute);require(value.length()<=4096,"Project text field is too long.");return value.toStdString();}
std::uint32_t checksum(const float* data,size_t size)
{
    auto crc=0xffffffffu;const auto* bytes=reinterpret_cast<const unsigned char*>(data);
    for(size_t i=0;i<size*sizeof(float);++i)
    {crc^=bytes[i];for(int b=0;b<8;++b)crc=(crc>>1)^(0xedb88320u&static_cast<unsigned>(-static_cast<int>(crc&1)));}
    return ~crc;
}
}
juce::Result ProjectFile::save(const juce::File& file,const ProjectSnapshot& state)
{
    try
    {
        Xml root("AuralisProject");root.setAttribute("version",formatVersion);
        std::vector<std::shared_ptr<const AudioData>> media;
        auto mediaId=[&](const std::shared_ptr<const AudioData>& source)
        {
            if(!source)return -1;
            const auto found=std::find(media.begin(),media.end(),source);
            if(found!=media.end())return static_cast<int>(found-media.begin());
            media.push_back(source);return static_cast<int>(media.size()-1);
        };
        auto* transport=root.createNewChildElement("Transport");
        transport->setAttribute("tempo",state.transport.tempo);transport->setAttribute("seconds",state.transport.seconds);
        transport->setAttribute("numerator",state.transport.numerator);transport->setAttribute("denominator",state.transport.denominator);
        transport->setAttribute("loop",state.transport.loop);transport->setAttribute("loopStart",state.transport.loopStartBeats);transport->setAttribute("loopEnd",state.transport.loopEndBeats);
        transport->setAttribute("record",state.transport.recordArmed);transport->setAttribute("metronome",state.transport.metronome);
        transport->setAttribute("selected",state.selected);transport->setAttribute("active",state.active);transport->setAttribute("activeAudio",state.activeAudio);
        transport->setAttribute("snap",state.snap);transport->setAttribute("view",state.view);
        auto* ui=root.createNewChildElement("View");
        ui->setAttribute("automation",state.ui.automation);ui->setAttribute("browser",state.ui.browser);ui->setAttribute("editor",state.ui.editor);
        ui->setAttribute("bar",state.ui.arrangementBar);ui->setAttribute("track",state.ui.trackHeight);ui->setAttribute("header",state.ui.header);ui->setAttribute("vertical",state.ui.vertical);ui->setAttribute("offset",state.ui.arrangementOffset);
        ui->setAttribute("pixels",state.ui.pianoPixels);ui->setAttribute("pianoOffset",state.ui.pianoOffset);ui->setAttribute("pitch",state.ui.pianoPitch);ui->setAttribute("row",state.ui.pianoRow);ui->setAttribute("control",state.ui.pianoControl);
        auto* channels=root.createNewChildElement("Channels");
        for(const auto& c:state.channels)
        {
            auto* x=channels->createNewChildElement("Channel");x->setAttribute("id",c.id);x->setAttribute("kind",static_cast<int>(c.kind));x->setAttribute("name",c.name);x->setAttribute("type",c.type);
            x->setAttribute("tint",juce::String(static_cast<juce::int64>(c.tint)));x->setAttribute("gain",c.gain);x->setAttribute("pan",c.pan);x->setAttribute("stereo",c.stereo);
            x->setAttribute("muted",c.muted);x->setAttribute("solo",c.solo);x->setAttribute("armed",c.armed);x->setAttribute("polarity",c.polarity);
            if(c.source){x->setAttribute("sourceName",c.source->name);x->setAttribute("sourcePath",c.source->path);}
            for(const auto& lane:c.automation)
            {
                auto* a=x->createNewChildElement("Automation");a->setAttribute("id",lane.id);a->setAttribute("device",lane.device);a->setAttribute("parameter",lane.parameter);a->setAttribute("name",lane.name);a->setAttribute("enabled",lane.enabled);a->setAttribute("log",lane.logarithmic);a->setAttribute("minimum",lane.minimum);a->setAttribute("maximum",lane.maximum);
                for(const auto& point:lane.points){auto* p=a->createNewChildElement("Point");setTick(*p,"time",point.time);p->setAttribute("value",point.value);p->setAttribute("curve",point.curve);}
            }
            for(const auto& d:c.devices)
            {
                auto* device=x->createNewChildElement("Device");device->setAttribute("id",d.id);device->setAttribute("kind",static_cast<int>(d.kind));device->setAttribute("bypass",d.bypass);device->setAttribute("media",mediaId(d.sample));
                if(d.kind==DeviceKind::external)
                {
                    require(d.parameterIds.size()==d.externalValues.size()&&d.parameterNames.size()==d.externalValues.size(),"Plugin parameter metadata is inconsistent.");
                    device->setAttribute("pluginName",d.pluginName);device->setAttribute("pluginInstrument",d.pluginInstrument);device->setAttribute("description",d.pluginDescription);
                    device->setAttribute("state",juce::MemoryBlock(d.pluginState.data(),d.pluginState.size()).toBase64Encoding());
                    for(size_t i=0;i<d.externalValues.size();++i){auto* p=device->createNewChildElement("ExternalParameter");p->setAttribute("key",d.parameterIds[i]);p->setAttribute("name",d.parameterNames[i]);p->setAttribute("value",d.externalValues[i]);}
                }
                const auto specs=deviceParameters(d.kind);
                for(size_t i=0;i<specs.size();++i){auto* p=device->createNewChildElement("Parameter");p->setAttribute("id",static_cast<int>(i));p->setAttribute("value",d.values[i]);}
                for(const auto& m:d.modulation){auto* mod=device->createNewChildElement("Modulation");mod->setAttribute("source",m.source);mod->setAttribute("target",m.target);mod->setAttribute("depth",m.amount);}
            }
        }
        auto* routes=root.createNewChildElement("Routes");
        for(const auto& r:state.routes){auto* x=routes->createNewChildElement("Route");x->setAttribute("source",r.source);x->setAttribute("destination",r.destination);x->setAttribute("amount",r.amount);}
        auto* clips=root.createNewChildElement("MidiClips");
        for(const auto& c:state.midi)
        {
            auto* x=clips->createNewChildElement("Clip");x->setAttribute("id",c.id);x->setAttribute("track",c.track);x->setAttribute("name",c.name);
            setTick(*x,"start",c.start);setTick(*x,"length",c.length);setTick(*x,"cycle",c.cycle);setTick(*x,"offset",c.offset);
            for(const auto& n:c.notes)
            {
                auto* note=x->createNewChildElement("Note");note->setAttribute("id",n.id);note->setAttribute("pitch",n.pitch);note->setAttribute("velocity",n.velocity);note->setAttribute("pan",n.pan);note->setAttribute("fine",n.finePitch);note->setAttribute("muted",n.muted);setTick(*note,"start",n.start);setTick(*note,"length",n.length);
            }
            for(const auto& e:c.events){auto* event=x->createNewChildElement("Event");setTick(*event,"time",e.time);event->setAttribute("target",static_cast<int>(e.target));event->setAttribute("value",e.value);}
        }
        auto* audio=root.createNewChildElement("AudioClips");
        for(const auto& c:state.audio)
        {
            auto* x=audio->createNewChildElement("Clip");x->setAttribute("id",c.id);x->setAttribute("track",c.track);x->setAttribute("name",c.name);x->setAttribute("media",mediaId(c.source));
            setTick(*x,"start",c.start);setTick(*x,"length",c.length);x->setAttribute("sourceStart",c.sourceStart);x->setAttribute("sourceEnd",c.sourceEnd);x->setAttribute("phase",c.phase);
            x->setAttribute("gain",c.gainDb);x->setAttribute("pitch",c.semitones);x->setAttribute("fadeIn",c.fadeIn);x->setAttribute("fadeOut",c.fadeOut);x->setAttribute("loop",c.loop);x->setAttribute("reverse",c.reverse);x->setAttribute("muted",c.muted);
        }
        auto* assets=root.createNewChildElement("Media");int64_t total=0;
        for(const auto& source:media)
        {
            require(source->left.size()==source->right.size(),"Media channel lengths do not match.");
            total+=static_cast<int64_t>(source->left.size())*8;require(total<=maximumMediaBytes,"Project exceeds the embedded media limit.");
            auto* x=assets->createNewChildElement("Asset");x->setAttribute("path",source->path);x->setAttribute("rate",source->sampleRate);x->setAttribute("frames",juce::String(static_cast<juce::int64>(source->left.size())));
            x->setAttribute("leftCrc",juce::String(static_cast<juce::int64>(checksum(source->left.data(),source->left.size()))));x->setAttribute("rightCrc",juce::String(static_cast<juce::int64>(checksum(source->right.data(),source->right.size()))));
        }
        const auto xml=root.toString();const auto bytes=xml.getNumBytesAsUTF8();require(bytes<=maximumMetadata,"Project metadata is too large.");
        juce::TemporaryFile temporary(file);auto output=temporary.getFile().createOutputStream();require(output!=nullptr,"Cannot create the temporary project file.");
        output->writeInt(0x41555031);output->writeInt(formatVersion);output->writeInt(static_cast<int>(bytes));output->write(xml.toRawUTF8(),bytes);
        for(const auto& source:media){output->write(source->left.data(),source->left.size()*sizeof(float));output->write(source->right.data(),source->right.size()*sizeof(float));}
        output->flush();const auto status=output->getStatus();output.reset();require(status.wasOk(),"Writing the project failed. The original was preserved.");
        ProjectSnapshot verified;const auto validation=load(temporary.getFile(),verified);require(validation.wasOk(),validation.getErrorMessage().toRawUTF8());
        require(temporary.overwriteTargetFileWithTemporary(),"Cannot replace the project file. The original was preserved.");
        return juce::Result::ok();
    }
    catch(const std::exception& error){return juce::Result::fail(error.what());}
}
juce::Result ProjectFile::load(const juce::File& file,ProjectSnapshot& destination)
{
    try
    {
        auto input=file.createInputStream();require(input!=nullptr,"Cannot open the project.");
        require(input->getTotalLength()<=maximumMediaBytes+maximumMetadata+12,"Project file is too large.");
        require(input->readInt()==0x41555031,"This is not an Auralis project.");require(input->readInt()==formatVersion,"Unsupported Auralis project version; the open session was preserved.");
        const int bytes=input->readInt();require(bytes>0&&bytes<=maximumMetadata&&bytes<=input->getTotalLength()-12,"Invalid project metadata length.");
        juce::MemoryBlock data(static_cast<size_t>(bytes));require(input->read(data.getData(),bytes)==bytes,"Project metadata is truncated.");
        auto root=juce::parseXML(juce::String::fromUTF8(static_cast<const char*>(data.getData()),bytes));require(root&&root->hasTagName("AuralisProject"),"Invalid project metadata.");
        require(integer(*root,"version",1,1)==formatVersion,"Unsupported project schema.");
        std::set<juce::String> sections;
        for(const auto* child:root->getChildIterator())
        {
            const auto tag=child->getTagName();
            require(tag=="Transport"||tag=="View"||tag=="Channels"||tag=="Routes"||tag=="MidiClips"||tag=="AudioClips"||tag=="Media","Unknown project section; refusing to discard it.");
            require(sections.insert(tag).second,"Duplicate project section.");
        }
        auto section=[&](const char* tag)->const Xml&{auto* x=root->getChildByName(tag);require(x!=nullptr,"Missing project section.");return *x;};
        ProjectSnapshot state;
        std::vector<std::shared_ptr<const AudioData>> media;int64_t total=0;
        for(const auto* x:section("Media").getChildIterator())
        {
            require(media.size()<audio::maximumVoices+devices::maximumTotalDevices,"Too many embedded audio assets.");
            auto source=std::make_shared<AudioData>();source->path=name(*x,"path");source->sampleRate=number(*x,"rate",8000,384000);
            const int frames=integer(*x,"frames",1,static_cast<int>(maximumMediaBytes/8));total+=static_cast<int64_t>(frames)*8;
            require(total<=maximumMediaBytes&&static_cast<int64_t>(frames)*8<=input->getTotalLength()-input->getPosition(),"Invalid or truncated embedded audio.");
            source->left.resize(frames);source->right.resize(frames);
            require(input->read(source->left.data(),frames*4)==frames*4&&input->read(source->right.data(),frames*4)==frames*4,"Truncated audio samples.");
            require(checksum(source->left.data(),frames)==static_cast<uint32_t>(number(*x,"leftCrc",0,4294967295.0))&&checksum(source->right.data(),frames)==static_cast<uint32_t>(number(*x,"rightCrc",0,4294967295.0)),"Embedded audio checksum failed.");
            for(int i=0;i<frames;++i)
            {require(std::isfinite(source->left[i])&&std::isfinite(source->right[i]),"Non-finite audio samples.");const auto b=static_cast<size_t>(i)*source->peaks.size()/frames;source->peaks[b]=std::max({source->peaks[b],std::abs(source->left[i]),std::abs(source->right[i])});}
            media.push_back(std::move(source));
        }
        require(input->getPosition()==input->getTotalLength(),"Unexpected trailing project data.");
        auto sourceFor=[&](const Xml& x){const int id=integer(x,"media",-1,static_cast<int>(media.size())-1);return id<0?std::shared_ptr<const AudioData>{}:media[id];};
        std::set<int> channelIds,deviceIds,clipIds,noteIds;
        for(const auto* x:section("Channels").getChildIterator())
        {
            require(state.channels.size()<devices::maximumChannels,"Too many channels.");ChannelState c;c.id=integer(*x,"id",0,devices::maximumChannels-1);
            require(channelIds.insert(c.id).second,"Duplicate channel identifier.");c.kind=static_cast<ChannelKind>(integer(*x,"kind",0,2));
            require((c.id==0)==(c.kind==ChannelKind::master),"Invalid Master channel.");c.name=name(*x,"name");c.type=name(*x,"type");
            c.tint=static_cast<uint32_t>(number(*x,"tint",0,4294967295.0));c.gain=number(*x,"gain",design::minimumGainDb,design::maximumGainDb);c.pan=number(*x,"pan",-100,100);c.stereo=number(*x,"stereo",-100,100);
            c.muted=flag(*x,"muted");c.solo=flag(*x,"solo");c.armed=flag(*x,"armed");c.polarity=flag(*x,"polarity");
            if(x->hasAttribute("sourcePath"))c.source=SourceState{name(*x,"sourceName"),name(*x,"sourcePath")};
            for(const auto* d:x->getChildIterator())
            {
                if(d->hasTagName("Automation"))
                {
                    require(c.automation.size()<project::maximumAutomationLanes,"Too many automation lanes.");AutomationLane lane;
                    lane.id=integer(*d,"id",1,1000000);lane.device=integer(*d,"device",-1,100000000);lane.parameter=integer(*d,"parameter",0,4095);lane.name=name(*d,"name");lane.enabled=flag(*d,"enabled");lane.logarithmic=flag(*d,"log");lane.minimum=number(*d,"minimum",-1e6,1e6);lane.maximum=number(*d,"maximum",lane.minimum,1e6);
                    require(lane.maximum>lane.minimum&&(!lane.logarithmic||lane.minimum>0),"Invalid automation scale.");
                    for(const auto* p:d->getChildIterator())
                    {require(lane.points.size()<project::maximumAutomationPoints,"Too many automation points.");const auto time=tick(*p,"time");require(lane.points.empty()||time>lane.points.back().time,"Unordered automation points.");lane.points.push_back({time,number(*p,"value",0,1),number(*p,"curve",-1,1)});}
                    c.automation.push_back(std::move(lane));continue;
                }
                require(d->hasTagName("Device"),"Unknown channel state; refusing to discard it.");
                require(c.devices.size()<devices::maximumDevices&&deviceIds.size()<devices::maximumTotalDevices,"Too many devices.");
                DeviceState device(static_cast<DeviceKind>(integer(*d,"kind",0,10)),integer(*d,"id",1,100000000));require(deviceIds.insert(device.id).second,"Duplicate device identifier.");device.bypass=flag(*d,"bypass");device.sample=sourceFor(*d);
                if(device.kind==DeviceKind::external)
                {
                    device.pluginName=name(*d,"pluginName");device.pluginInstrument=flag(*d,"pluginInstrument");device.pluginDescription=name(*d,"description");
                    juce::MemoryBlock block;require(block.fromBase64Encoding(d->getStringAttribute("state"))&&block.getSize()<=16*1024*1024,"Invalid plugin state.");const auto* stateBytes=static_cast<const uint8_t*>(block.getData());if(block.getSize()>0)device.pluginState.assign(stateBytes,stateBytes+block.getSize());
                }
                const auto specs=deviceParameters(device.kind);std::set<int> parameters;
                for(const auto* p:d->getChildIterator())
                {
                    if(p->hasTagName("ExternalParameter")&&device.kind==DeviceKind::external)
                    {require(device.externalValues.size()<project::maximumPluginParameters,"Too many plugin parameters.");device.parameterIds.push_back(name(*p,"key"));device.parameterNames.push_back(name(*p,"name"));device.externalValues.push_back(number(*p,"value",0,1));}
                    else if(p->hasTagName("Parameter")){const int id=integer(*p,"id",0,static_cast<int>(specs.size())-1);require(parameters.insert(id).second,"Duplicate parameter.");device.values[id]=number(*p,"value",specs[id].minimum,specs[id].maximum);}
                    else if(p->hasTagName("Modulation")){require(device.modulation.size()<devices::maximumModulations,"Too many modulation assignments.");device.modulation.push_back({integer(*p,"source",0,6),integer(*p,"target",0,60),number(*p,"depth",-1,1)});}
                    else require(false,"Unknown device state; refusing to discard it.");
                }
                require(parameters.size()==specs.size(),"Incomplete device parameters.");c.devices.push_back(std::move(device));
            }
            std::set<int> laneIds;
            for(const auto& lane:c.automation)
            {
                require(laneIds.insert(lane.id).second,"Duplicate automation lane identifier.");
                double low=0,high=1;bool logarithmic=false;
                if(lane.device<0)
                {
                    if(lane.parameter==0){low=design::minimumGainDb;high=design::maximumGainDb;}
                    else if(lane.parameter==1||lane.parameter==2){low=-100;high=100;}
                    else require(lane.parameter>=100&&lane.parameter<100+devices::maximumChannels,"Invalid mixer automation parameter.");
                }
                else
                {
                    const auto target=std::find_if(c.devices.begin(),c.devices.end(),[&](const auto& device){return device.id==lane.device;});
                    if(target==c.devices.end())continue; // Preserve orphaned lanes for recovery; the renderer ignores them.
                    if(target->kind==DeviceKind::external)require(lane.parameter<static_cast<int>(target->externalValues.size()),"Invalid plugin automation parameter.");
                    else
                    {
                        const auto specs=deviceParameters(target->kind);
                        require(lane.parameter<static_cast<int>(specs.size()),"Invalid device automation parameter.");
                        low=specs[lane.parameter].minimum;high=specs[lane.parameter].maximum;logarithmic=specs[lane.parameter].logarithmic;
                    }
                }
                require(lane.minimum==low&&lane.maximum==high&&lane.logarithmic==logarithmic,"Automation range does not match its target.");
            }
            state.channels.push_back(std::move(c));
        }
        require(channelIds.contains(0),"Project has no Master.");
        std::sort(state.channels.begin(),state.channels.end(),[](const auto& a,const auto& b){return a.id<b.id;});
        auto channel=[&](int id)->const ChannelState&{for(const auto& c:state.channels)if(c.id==id)return c;throw std::runtime_error("Clip or route references a missing channel.");};
        std::set<std::pair<int,int>> routeIds;
        for(const auto* x:section("Routes").getChildIterator())
        {
            SendRoute r{integer(*x,"source",1,devices::maximumChannels-1),integer(*x,"destination",0,devices::maximumChannels-1),number(*x,"amount",0,1)};
            channel(r.source);channel(r.destination);require(r.source!=r.destination&&routeIds.insert({r.source,r.destination}).second,"Invalid route.");state.routes.push_back(r);
        }
        auto pending=channelIds;
        while(!pending.empty())
        {
            auto it=std::find_if(pending.begin(),pending.end(),[&](int id){return std::none_of(state.routes.begin(),state.routes.end(),[&](const auto& r){return r.destination==id&&pending.contains(r.source);});});
            require(it!=pending.end(),"Project contains a routing feedback cycle.");pending.erase(it);
        }
        for(const auto* x:section("MidiClips").getChildIterator())
        {
            require(state.midi.size()<editing::maximumClips,"Too many MIDI clips.");MidiClip c;c.id=integer(*x,"id",1,100000000);require(clipIds.insert(c.id).second,"Duplicate clip identifier.");c.track=integer(*x,"track",1,devices::maximumChannels-1);require(channel(c.track).type=="MIDI","MIDI clip on a non-instrument channel.");c.name=name(*x,"name");c.start=tick(*x,"start");c.length=tick(*x,"length",1);c.cycle=tick(*x,"cycle",1);c.offset=tick(*x,"offset");require(c.offset<c.cycle&&c.start+c.length<=editing::maximumTime,"Invalid MIDI clip range.");
            for(const auto* n:x->getChildIterator())
            {
                if(n->hasTagName("Note"))
                {
                    require(c.notes.size()<editing::maximumNotes,"Too many notes.");MidiNote note;note.id=integer(*n,"id",1,100000000);note.pitch=integer(*n,"pitch",0,127);note.velocity=integer(*n,"velocity",0,127);note.pan=integer(*n,"pan",-100,100);note.finePitch=integer(*n,"fine",-1200,1200);note.start=tick(*n,"start");note.length=tick(*n,"length",1);note.muted=flag(*n,"muted");require(note.start+note.length<=c.cycle,"Note outside clip cycle.");c.notes.push_back(note);
                }
                else if(n->hasTagName("Event")){require(c.events.size()<editing::maximumNotes,"Too many MIDI events.");c.events.push_back({tick(*n,"time"),static_cast<EventTarget>(integer(*n,"target",0,2)),number(*n,"value",-16384,16384)});}
                else require(false,"Unknown MIDI content.");
            }
            state.midi.push_back(std::move(c));
        }
        for(const auto* x:section("AudioClips").getChildIterator())
        {
            require(state.audio.size()<audio::maximumVoices,"Too many audio clips.");AudioClip c;c.id=integer(*x,"id",1,100000000);require(clipIds.insert(c.id).second,"Duplicate clip identifier.");c.track=integer(*x,"track",1,devices::maximumChannels-1);require(channel(c.track).type=="AUDIO","Audio clip on a non-audio channel.");c.name=name(*x,"name");c.source=sourceFor(*x);require(c.source!=nullptr,"Audio clip has no embedded media.");
            c.start=tick(*x,"start");c.length=tick(*x,"length",1);require(c.start+c.length<=editing::maximumTime,"Audio clip extends past the timeline.");c.sourceStart=number(*x,"sourceStart",0,c.source->seconds());c.sourceEnd=number(*x,"sourceEnd",c.sourceStart,c.source->seconds());c.phase=number(*x,"phase",0,1e9);c.gainDb=number(*x,"gain",-60,24);c.semitones=number(*x,"pitch",-48,48);c.fadeIn=number(*x,"fadeIn",0,1e9);c.fadeOut=number(*x,"fadeOut",0,1e9);c.loop=flag(*x,"loop");c.reverse=flag(*x,"reverse");c.muted=flag(*x,"muted");state.audio.push_back(std::move(c));
        }
        const auto& t=section("Transport");state.transport.tempo=number(t,"tempo",design::minTempo,design::maxTempo);state.transport.seconds=number(t,"seconds",0,1e9);state.transport.numerator=integer(t,"numerator",1,32);state.transport.denominator=integer(t,"denominator",1,32);require((state.transport.denominator&(state.transport.denominator-1))==0,"Invalid time signature.");state.transport.loop=flag(t,"loop");state.transport.loopStartBeats=number(t,"loopStart",0,1e9);state.transport.loopEndBeats=number(t,"loopEnd",state.transport.loopStartBeats,1e9);state.transport.recordArmed=flag(t,"record");state.transport.metronome=flag(t,"metronome");state.selected=integer(t,"selected",0,devices::maximumChannels-1);channel(state.selected);state.active=integer(t,"active",0,100000000);state.activeAudio=integer(t,"activeAudio",0,100000000);state.snap=integer(t,"snap",0,11);state.view=integer(t,"view",0,2);
        if(const auto* ui=root->getChildByName("View"))
        {
            state.ui.automation=flag(*ui,"automation");state.ui.browser=integer(*ui,"browser",100,4000);state.ui.editor=integer(*ui,"editor",50,4000);
            state.ui.arrangementBar=integer(*ui,"bar",32,1280);state.ui.trackHeight=integer(*ui,"track",design::trackHeight,design::trackHeight*2);state.ui.header=integer(*ui,"header",100,1000);state.ui.vertical=integer(*ui,"vertical",0,1000000);state.ui.arrangementOffset=number(*ui,"offset",0,design::timelineBars);
            state.ui.pianoPixels=number(*ui,"pixels",1,4000);state.ui.pianoOffset=number(*ui,"pianoOffset",0,editing::maximumTime);state.ui.pianoPitch=number(*ui,"pitch",0,128);state.ui.pianoRow=integer(*ui,"row",1,200);state.ui.pianoControl=integer(*ui,"control",1,4000);
        }
        destination=std::move(state);return juce::Result::ok();
    }
    catch(const std::exception& error){return juce::Result::fail(error.what());}
}
}
