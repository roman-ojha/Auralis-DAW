#include "audio/PluginHost.h"
#include <iostream>
int verifyHost(const juce::String& path)
{
    using namespace auralis;
    auto check=[](bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);};
    try
    {
        juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> found;format.findAllTypesForFile(found,juce::File(path).getFullPathName());
        check(found.size()==1,"Fixture scan failed");
        DeviceState state(DeviceKind::external);state.pluginDescription=found[0]->createXml()->toString().toStdString();
        juce::String error;auto plugin=NativePlugin::create(state,48000,error);check(plugin!=nullptr,error.toRawUTF8());
        const auto gain=std::find(state.parameterNames.begin(),state.parameterNames.end(),"Fixture gain");
        check(gain!=state.parameterNames.end(),"Stable parameter discovery failed");const int gainIndex=static_cast<int>(gain-state.parameterNames.begin());
        plugin->parameter(gainIndex,.25);float left[64],right[64];std::fill_n(left,64,1.f);std::fill_n(right,64,-.5f);
        plugin->process(left,right,64,{},120,0,true);
        check(std::abs(left[63]-.25f)<1e-6&&std::abs(right[63]+.125f)<1e-6,"Native VST3 stereo processing failed");
        state.pluginState=plugin->saveState();plugin.reset();
        plugin=NativePlugin::create(state,44100,error);check(plugin!=nullptr,error.toRawUTF8());
        std::fill_n(left,64,1.f);std::fill_n(right,64,1.f);plugin->process(left,right,17,{},137,2,true);
        check(std::abs(left[16]-.25f)<1e-6,"Saved VST3 state or variable block restore failed");
        plugin->parameter(gainIndex,.75);std::fill_n(left,64,1.f);plugin->process(left,right,64,{},120,0,true);
        check(std::abs(left[63]-.75f)<1e-6,"Native parameter automation failed");
        plugin.reset();std::cout<<"VST3 scan, stereo DSP, parameter control, state restoration and lifecycle passed.\n";return 0;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
