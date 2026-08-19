#pragma once
#include "HostedProcessor.h"
#include "model/DeviceState.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

namespace auralis
{
class NativePlugin final : public HostedProcessor,private juce::AudioProcessorListener,private juce::AudioPlayHead
{
public:
    static std::shared_ptr<NativePlugin> create(DeviceState&,double,juce::String&);
    ~NativePlugin() override;
    void prepare(double) override;
    void process(float*,float*,int,std::span<const HostMidiEvent>,double,double,bool) override;
    void parameter(int,double) override;
    void reset() override { processor->reset(); }
    int latency() const override {return processor->getLatencySamples();}
    std::vector<uint8_t> saveState() override;
    void showEditor();
    void closeEditor();
    void poll(const std::function<void(int,double)>&);
private:
    explicit NativePlugin(std::unique_ptr<juce::AudioPluginInstance>);
    std::unique_ptr<juce::AudioPluginInstance> processor;
    std::unique_ptr<juce::DocumentWindow> editor;
    juce::AudioBuffer<float> buffer{2,64};
    juce::MidiBuffer midi;
    std::vector<int> indices,reverseIndices;
    std::unique_ptr<std::atomic<double>[]> changes;
    double rate=48000,bpm=120,seconds=0;
    bool playing=false,prepared=false;
    static thread_local bool fromHost;
    void audioProcessorParameterChanged(juce::AudioProcessor*,int,float) override;
    void audioProcessorChanged(juce::AudioProcessor*,const ChangeDetails&) override {}
    juce::Optional<PositionInfo> getPosition() const override;
};
}
