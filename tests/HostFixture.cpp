#include <juce_audio_processors/juce_audio_processors.h>
// Repository-owned VST3 fixture: no installed third-party binary is required
// to exercise scanning, parameter IDs, state restoration and native editors.
class HostFixture final : public juce::AudioProcessor
{
public:
    HostFixture():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo()).withOutput("Output",juce::AudioChannelSet::stereo()))
    {addParameter(gain=new juce::AudioParameterFloat(juce::ParameterID{"gain",1},"Fixture gain",0.0f,1.0f,.5f));}
    const juce::String getName() const override{return "Auralis Host Fixture";}
    void prepareToPlay(double,int) override{}
    void releaseResources() override{}
    void processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) override{buffer.applyGain(gain->get());}
    bool isBusesLayoutSupported(const BusesLayout& layout) const override{return layout.getMainInputChannelSet()==juce::AudioChannelSet::stereo()&&layout.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
    bool hasEditor() const override{return true;}
    juce::AudioProcessorEditor* createEditor() override{return new juce::GenericAudioProcessorEditor(*this);}
    double getTailLengthSeconds() const override{return 0;}
    bool acceptsMidi() const override{return false;}
    bool producesMidi() const override{return false;}
    int getNumPrograms() override{return 1;}
    int getCurrentProgram() override{return 0;}
    void setCurrentProgram(int) override{}
    const juce::String getProgramName(int) override{return "Default";}
    void changeProgramName(int,const juce::String&) override{}
    void getStateInformation(juce::MemoryBlock& block) override{const float value=gain->get();block.replaceAll(&value,sizeof(value));}
    void setStateInformation(const void* data,int size) override{if(size==sizeof(float)){float value;std::memcpy(&value,data,sizeof(value));if(std::isfinite(value))*gain=std::clamp(value,0.0f,1.0f);}}
private:juce::AudioParameterFloat* gain=nullptr;
};
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new HostFixture();}
