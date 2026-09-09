#include "Workspace.h"
namespace auralis
{
void Workspace::refreshPlugins()
{
    std::vector<Browser::PluginEntry> entries;
    for(const auto& record:pluginSettings.plugins)if(record.favourite)
        entries.push_back({record.description.name,record.description.manufacturerName+" / VST3",record.description.createIdentifierString()});
    browser.setPlugins(std::move(entries));
}
void Workspace::addPlugin(const juce::String& identity,int track)
{
    const auto found=std::find_if(pluginSettings.plugins.begin(),pluginSettings.plugins.end(),[&](const auto& p){return p.description.createIdentifierString()==identity;});
    if(found==pluginSettings.plugins.end())return;
    const bool instrument=found->description.isInstrument;
    if(track<0)track=tracks.addTrack(instrument);
    auto* channel=tracks.find(track);if(!channel)return;
    if(instrument&&(channel->type!="MIDI"||std::any_of(channel->devices.begin(),channel->devices.end(),[](const auto& d){return d.instrument();})))
    {browser.infoView.show({"Choose an empty instrument chain","This plugin is an instrument. Drop it below the arrangement to create a MIDI channel, or onto an empty MIDI channel."});return;}
    DeviceState plugin(DeviceKind::external);plugin.pluginName=found->description.name.toStdString();plugin.pluginInstrument=instrument;plugin.pluginDescription=found->description.createXml()->toString().toStdString();
    juce::String error;plugin.hosted=NativePlugin::create(plugin,audioOutput.currentRate(),error);
    if(!plugin.hosted){juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Plugin load failed",error);return;}
    if(!tracks.addDevice(track,DeviceKind::external)){browser.infoView.show({"Device limit","This channel or project has reached the device limit."});return;}
    channel=tracks.find(track);const int id=channel->devices.back().id;plugin.id=id;channel->devices.back()=std::move(plugin);
    if(instrument)std::rotate(channel->devices.begin(),channel->devices.end()-1,channel->devices.end());
    devices.invalidate();tracks.select(track);tracks.notifyDevices();
    if(instrument){const auto audioError=audioOutput.open();if(audioError.isNotEmpty())browser.infoView.show({"Audio output unavailable",audioError});}
}
void Workspace::restorePlugins()
{
    juce::StringArray unavailable;
    for(const auto& c:tracks.all())if(auto* channel=tracks.find(c.id))for(auto& d:channel->devices)if(d.kind==DeviceKind::external)
    {
        juce::String error;d.hosted=NativePlugin::create(d,audioOutput.currentRate(),error);
        if(!d.hosted)unavailable.add(juce::String(d.pluginName)+": "+error);
    }
    if(!unavailable.isEmpty())juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Unavailable plugins",unavailable.joinIntoString("\n")+"\n\nTheir saved state and automation have been preserved. Missing effects pass audio through; missing instruments are silent.");
}
}
