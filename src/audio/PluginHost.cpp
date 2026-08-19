#include "PluginHost.h"
namespace auralis
{
thread_local bool NativePlugin::fromHost=false;
namespace
{
class PluginWindow final : public juce::DocumentWindow
{
public:
    explicit PluginWindow(juce::AudioPluginInstance& plugin):DocumentWindow(plugin.getName(),juce::Colour(0xff182124),closeButton)
    {
        setUsingNativeTitleBar(true);
        auto* content=plugin.createEditorIfNeeded();if(!content)content=new juce::GenericAudioProcessorEditor(plugin);
        setContentOwned(content,true);setResizable(content->isResizable(),false);centreWithSize(content->getWidth(),content->getHeight());setVisible(true);
    }
    void closeButtonPressed() override {setVisible(false);}
};
std::string parameterId(juce::AudioProcessorParameter* parameter,int index)
{
    if(auto* identified=dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter))return identified->paramID.toStdString();
    if(auto* hosted=dynamic_cast<juce::HostedAudioProcessorParameter*>(parameter))return hosted->getParameterID().toStdString();
    return "index:"+std::to_string(index);
}
}
NativePlugin::NativePlugin(std::unique_ptr<juce::AudioPluginInstance> instance):processor(std::move(instance))
{midi.ensureSize(1024*1024);processor->setPlayHead(this);}
NativePlugin::~NativePlugin()
{editor.reset();processor->removeListener(this);processor->setPlayHead(nullptr);if(prepared)processor->releaseResources();}
std::shared_ptr<NativePlugin> NativePlugin::create(DeviceState& state,double sampleRate,juce::String& error)
{
    auto xml=juce::parseXML(juce::String(state.pluginDescription));juce::PluginDescription description;
    if(!xml||!description.loadFromXml(*xml)){error="Invalid saved plugin description.";return {};}
    juce::AudioPluginFormatManager formats;formats.addFormat(std::make_unique<juce::VST3PluginFormat>());
    auto instance=formats.createPluginInstance(description,sampleRate,64,error);if(!instance)return {};
    instance->enableAllBuses();auto layout=instance->getBusesLayout();
    for(int i=0;i<layout.inputBuses.size();++i)layout.inputBuses.getReference(i)=i==0?juce::AudioChannelSet::stereo():juce::AudioChannelSet::disabled();
    for(int i=0;i<layout.outputBuses.size();++i)layout.outputBuses.getReference(i)=i==0?juce::AudioChannelSet::stereo():juce::AudioChannelSet::disabled();
    if(!instance->setBusesLayout(layout)||instance->getTotalNumOutputChannels()!=2||instance->getTotalNumInputChannels()>2)
    {error="This host currently requires a stereo output and at most a stereo input.";return {};}
    auto runtime=std::shared_ptr<NativePlugin>(new NativePlugin(std::move(instance)));
    if(!state.pluginState.empty())runtime->processor->setStateInformation(state.pluginState.data(),static_cast<int>(state.pluginState.size()));
    const auto& parameters=runtime->processor->getParameters();
    if(parameters.size()>4096){error="Plugin exceeds the 4096-parameter host limit.";return {};}
    runtime->reverseIndices.resize(parameters.size(),-1);runtime->changes=std::make_unique<std::atomic<double>[]>(parameters.size());
    for(int i=0;i<parameters.size();++i)runtime->changes[i].store(-1);
    if(state.parameterIds.empty())
    {
        for(int i=0;i<parameters.size();++i)
        {state.parameterIds.push_back(parameterId(parameters[i],i));state.parameterNames.push_back(parameters[i]->getName(128).toStdString());state.externalValues.push_back(parameters[i]->getValue());}
    }
    for(size_t saved=0;saved<state.parameterIds.size();++saved)
    {
        int found=-1;for(int i=0;i<parameters.size();++i)if(parameterId(parameters[i],i)==state.parameterIds[saved]){found=i;break;}
        runtime->indices.push_back(found);if(found>=0)runtime->reverseIndices[found]=static_cast<int>(saved);
    }
    runtime->prepare(sampleRate);runtime->processor->addListener(runtime.get());return runtime;
}
void NativePlugin::prepare(double sampleRate)
{
    if(prepared)processor->releaseResources();rate=sampleRate;
    processor->setRateAndBufferSizeDetails(rate,64);processor->prepareToPlay(rate,64);prepared=true;
}
void NativePlugin::process(float* left,float* right,int count,std::span<const HostMidiEvent> events,double tempo,double position,bool running)
{
    bpm=tempo;seconds=position;playing=running;midi.clear();
    for(const auto& event:events)
        midi.addEvent(event.on?juce::MidiMessage::noteOn(1,event.pitch,event.velocity):juce::MidiMessage::noteOff(1,event.pitch),event.offset);
    buffer.copyFrom(0,0,left,count);buffer.copyFrom(1,0,right,count);
    float* pointers[]={buffer.getWritePointer(0),buffer.getWritePointer(1)};
    juce::AudioBuffer<float> view(pointers,2,count);processor->processBlock(view,midi);
    std::copy_n(buffer.getReadPointer(0),count,left);std::copy_n(buffer.getReadPointer(1),count,right);
}
void NativePlugin::parameter(int index,double value)
{
    if(index<0||index>=static_cast<int>(indices.size())||indices[index]<0)return;
    auto* target=processor->getParameters()[indices[index]];
    if(std::abs(target->getValue()-value)<1e-7)return;
    fromHost=true;target->setValue(static_cast<float>(value));fromHost=false;
}
void NativePlugin::audioProcessorParameterChanged(juce::AudioProcessor*,int index,float value)
{if(!fromHost&&index>=0&&index<static_cast<int>(reverseIndices.size()))changes[index].store(value,std::memory_order_relaxed);}
void NativePlugin::poll(const std::function<void(int,double)>& changed)
{for(size_t i=0;i<reverseIndices.size();++i){const auto value=changes[i].exchange(-1);if(value>=0&&reverseIndices[i]>=0)changed(reverseIndices[i],value);}}
std::vector<uint8_t> NativePlugin::saveState()
{
    juce::MemoryBlock block;processor->getStateInformation(block);
    if(block.getSize()>16*1024*1024)throw std::runtime_error("Plugin state exceeds 16 MiB.");
    const auto* data=static_cast<const uint8_t*>(block.getData());return {data,data+block.getSize()};
}
void NativePlugin::showEditor(){if(!editor)editor=std::make_unique<PluginWindow>(*processor);editor->setVisible(true);editor->toFront(true);}
void NativePlugin::closeEditor(){editor.reset();}
juce::Optional<juce::AudioPlayHead::PositionInfo> NativePlugin::getPosition() const
{
    PositionInfo result;result.setBpm(bpm);result.setTimeInSeconds(seconds);result.setTimeInSamples(static_cast<int64_t>(seconds*rate));result.setPpqPosition(seconds*bpm/60);result.setIsPlaying(playing);return result;
}
}
