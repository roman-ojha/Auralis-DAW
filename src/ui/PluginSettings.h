#pragma once
#include "Theme.h"
#include "ContextHelp.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace auralis
{
struct PluginRecord {juce::PluginDescription description;bool favourite=false;};
class PluginSettings final : public juce::Component,private juce::ListBoxModel
{
public:
    PluginSettings();
    ~PluginSettings() override;
    std::vector<PluginRecord> plugins;
    std::function<void()> onChanged;
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    juce::TextEditor paths;
    juce::TextButton addFolder,scan,cancel,retry;
    juce::StringArray quarantine;
    juce::Label status;
    juce::ListBox list{"Discovered plugins",this};
    juce::ThreadPool worker{1};std::atomic<bool> cancelled{false};
    std::unique_ptr<juce::FileChooser> chooser;
    juce::File preferences;
    void scanPlugins();void save();
    int getNumRows() override{return static_cast<int>(plugins.size());}
    void paintListBoxItem(int,juce::Graphics&,int,int,bool) override;
    void listBoxItemClicked(int,const juce::MouseEvent&) override;
};
class PluginSettingsWindow final : public juce::DocumentWindow
{
public:
    explicit PluginSettingsWindow(PluginSettings& content):DocumentWindow("Auralis Plug-In Settings",colour(design::colour::background),closeButton)
    {setUsingNativeTitleBar(true);setContentNonOwned(&content,true);setResizable(true,false);setResizeLimits(620,440,1400,1000);centreWithSize(780,570);setVisible(true);}
    void closeButtonPressed() override{setVisible(false);}
};
}
