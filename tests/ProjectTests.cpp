#include "model/ProjectFile.h"
#include "audio/SignalAnalysis.h"
#include <iostream>
#include <stdexcept>
using namespace auralis;
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(int argc,char** argv)
{
    try
    {
        MixerState mixer;MidiProject document;mixer.rename(1,"Lead / preserved");mixer.addDevice(1,DeviceKind::synth);mixer.addDevice(4,DeviceKind::delay);
        mixer.find(1)->devices[0].values[7]=.31;mixer.find(1)->devices[0].modulation.push_back({3,39,.25});
        const auto send=mixer.addSend();mixer.toggleRoute(1,send);mixer.sendAmount(1,send,.42);
        mixer.automationMode=true;mixer.automationTime=0;mixer.gain(1,-30);mixer.automationTime=1920;mixer.gain(1,-3);mixer.find(1)->automation[0].points[0].curve=.6;
        const int clipId=document.create(1,960);document.addNote(*document.clip(clipId),0,64,480);document.clip(clipId)->events.push_back({0,EventTarget::pan,-.3});
        auto source=std::make_shared<AudioData>();source->sampleRate=48000;source->path="missing-source.wav";source->left.assign(4800,.25f);source->right.assign(4800,-.125f);
        AudioClip audio;audio.id=document.freshClipId();audio.name="Embedded audio";audio.source=source;audio.sourceEnd=.1;audio.fadeIn=.02;audio.reverse=true;audio.loop=true;document.audioClips.push_back(audio);
        auto copy=audio;copy.id=document.freshClipId();copy.gainDb=-8;document.audioClips.push_back(copy);
        mixer.addDevice(2,DeviceKind::external);auto& plugin=mixer.find(2)->devices.back();plugin.pluginName="Missing test plugin";plugin.pluginDescription="<PLUGIN name=\"Missing\"/>";plugin.pluginInstrument=true;plugin.pluginState={0,1,255,32};plugin.parameterIds={"stable-cutoff"};plugin.parameterNames={"Cutoff"};plugin.externalValues={.73};
        ProjectSnapshot expected;expected.channels=mixer.all();expected.routes=mixer.sends();expected.midi=document.clips;expected.audio=document.audioClips;expected.transport.tempo=138.5;expected.transport.loop=true;expected.transport.loopStartBeats=2;expected.transport.loopEndBeats=6;expected.active=clipId;expected.ui.automation=true;expected.ui.arrangementBar=320;expected.ui.pianoPixels=160;
        juce::TemporaryFile file(".aup");auto result=ProjectFile::save(file.getFile(),expected);check(result.wasOk(),result.getErrorMessage().toRawUTF8());
        ProjectSnapshot actual;result=ProjectFile::load(file.getFile(),actual);check(result.wasOk(),result.getErrorMessage().toRawUTF8());
        check(actual.channels.size()==expected.channels.size()&&actual.routes.size()==expected.routes.size(),"Channel and route round trip");
        check(actual.channels[1].name=="Lead / preserved"&&actual.channels[1].devices[0].values[7]==.31,"Channel names and device parameter round trip");
        check(actual.channels[1].devices[0].modulation[0].source==3&&actual.channels[1].automation[0].points[0].curve==.6,"Modulation and automation curves round trip");
        check(actual.channels[2].devices[0].pluginState==plugin.pluginState&&actual.channels[2].devices[0].parameterIds==plugin.parameterIds,"Unavailable plugin state and stable parameter IDs preserved");
        check(actual.audio[0].source==actual.audio[1].source&&actual.audio[0].source->left==source->left&&actual.audio[1].gainDb==-8,"Embedded audio deduplicated, sample data exact, edits independent");
        check(actual.midi[0].notes[0].pitch==64&&actual.midi[0].events[0].value==-.3&&actual.transport.tempo==138.5,"MIDI events and transport round trip");
        check(actual.ui.automation&&actual.ui.arrangementBar==320&&actual.ui.pianoPixels==160,"Editor view state round trip");
        juce::MemoryBlock bytes;file.getFile().loadFileAsData(bytes);const auto original=bytes;
        static_cast<unsigned char*>(bytes.getData())[bytes.getSize()-1]^=1;file.getFile().replaceWithData(bytes.getData(),bytes.getSize());
        check(ProjectFile::load(file.getFile(),actual).failed()&&actual.transport.tempo==138.5,"Corrupt media rejected without altering destination");
        file.getFile().replaceWithData(original.getData(),original.getSize()/2);
        check(ProjectFile::load(file.getFile(),actual).failed(),"Truncated file rejected");
        auto broken=expected;broken.channels[1].gain=std::numeric_limits<double>::infinity();
        result=ProjectFile::save(file.getFile(),broken);
        check(result.failed()||ProjectFile::load(file.getFile(),actual).failed(),"Non-finite metadata rejected");
        file.getFile().replaceWithData(original.getData(),original.getSize());
        broken=expected;broken.channels[1].automation[0].maximum=100000;
        check(ProjectFile::save(file.getFile(),broken).failed(),"Unsafe automation ranges rejected");
        juce::MemoryBlock preserved;file.getFile().loadFileAsData(preserved);check(preserved==original,"Failed save preserves original bytes");
        broken=expected;broken.channels[2].devices[0].parameterNames.clear();
        check(ProjectFile::save(file.getFile(),broken).failed(),"Inconsistent external metadata rejected safely");
        AutomationLane lane;lane.minimum=-60;lane.maximum=6;lane.setPoint(0,-60);lane.setPoint(960,6);
        check(std::abs(lane.valueAt(480)+27)<1e-9,"Linear automation midpoint");lane.points[0].curve=1;
        check(lane.valueAt(480)<-50&&lane.valueAt(960)==6,"Curved segment and exact endpoint");
        SignalAnalysis analysis;
        for(int i=0;i<2048;++i){const double sine=std::sin(2*devices::pi*1000*i/48000);analysis.push(sine,-sine,1,0,48000);}
        const auto signal=analysis.consume();const auto peak=std::max_element(signal.spectrum.begin(),signal.spectrum.end())-signal.spectrum.begin();
        const double hz=20*std::pow(1000.0,static_cast<double>(peak)/SignalAnalysis::bands);
        check(hz>850&&hz<1150&&signal.output>.99,"Stereo FFT detects actual 1 kHz signal without phase cancellation");
        if(argc>=2)
        {
            MixerState demo;MidiProject notes;demo.addDevice(1,DeviceKind::synth);demo.addDevice(1,DeviceKind::equalizer);demo.addDevice(1,DeviceKind::compressor);demo.addDevice(1,DeviceKind::delay);
            demo.find(1)->devices[2].values[0]=-30;demo.gain(0,-24);demo.rename(1,"Visual verification");
            const int id=notes.create(1,0);notes.addNote(*notes.clip(id),0,60,notes.barTicks-15);
            ProjectSnapshot visual;visual.channels=demo.all();visual.routes=demo.sends();visual.midi=notes.clips;visual.active=id;visual.transport.loop=true;visual.transport.loopEndBeats=4;
            if(argc>=3)if(auto xml=juce::parseXML(juce::File(juce::String::fromUTF8(argv[2]))))if(auto* entry=xml->getFirstChildElement())
            {
                DeviceState fixture(DeviceKind::external,20);fixture.pluginName="Auralis Host Fixture";fixture.pluginDescription=entry->toString().toStdString();visual.channels[1].devices.push_back(std::move(fixture));
            }
            const auto written=ProjectFile::save(juce::File(juce::String::fromUTF8(argv[1])),visual);check(written.wasOk(),written.getErrorMessage().toRawUTF8());
        }
        std::cout<<"Project round trips, embedded audio CRC, invalid inputs, missing plugin state, automation curves and FFT passed.\n";
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
