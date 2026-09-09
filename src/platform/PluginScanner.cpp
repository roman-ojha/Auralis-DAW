#include <juce_audio_processors/juce_audio_processors.h>
#if AURALIS_HOST_VERIFICATION
int verifyHost(const juce::String&);
#endif
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if(argc!=3)return 2;
#if AURALIS_HOST_VERIFICATION
    if(juce::String(argv[1])=="--verify")return verifyHost(juce::String::fromUTF8(argv[2]));
#endif
    juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> plugins;
    format.findAllTypesForFile(plugins,juce::String::fromUTF8(argv[1]));
    juce::XmlElement result("Plugins");
    for(const auto* description:plugins)result.addChildElement(description->createXml().release());
    return result.writeTo(juce::File(juce::String::fromUTF8(argv[2])))?0:3;
}
