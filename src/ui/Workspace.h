#pragma once
#include "Browser.h"
#include "Transport.h"
#include "Arrangement.h"
#include "ApplicationMenu.h"
#include "Mixer.h"
#include "PianoRoll.h"
#include "ShortcutWindow.h"
#include "AudioEditor.h"
#include "audio/AudioOutput.h"
#include "DeviceArea.h"
#include "model/ProjectFile.h"
#include "PluginSettings.h"
#include "audio/PluginHost.h"
namespace auralis
{
class Workspace final : public juce::Component, private juce::Timer, private juce::KeyListener, public juce::DragAndDropContainer
{
public:
    Workspace();
    ~Workspace() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    void requestClose(std::function<void()>);
    void openProject(const juce::File&);
    std::function<void(const juce::String&)> onTitle;

private:
    PluginSettings pluginSettings;
    std::unique_ptr<PluginSettingsWindow> pluginWindow;
    void refreshPlugins();
    void addPlugin(const juce::String&,int);
    void restorePlugins();
    ProjectSnapshot snapshot() const;
    void captureProject(std::function<void(ProjectSnapshot)>);
    std::vector<std::shared_ptr<HostedProcessor>> exportPlugins;
    void applyProject(ProjectSnapshot);
    void projectCommand(int);
    void saveProject(bool,std::function<void()> after={});
    void updateTitle();
    bool projectShortcut(const juce::KeyPress&);
    bool keyPressed(const juce::KeyPress& key,juce::Component*) override {return projectShortcut(key);}
    juce::Component::SafePointer<juce::Component> shortcutFocus;
    void exportAudio();
    juce::File projectFile;
    std::unique_ptr<juce::FileChooser> projectChooser;
    juce::ThreadPool projectWorker{1};
    bool projectBusy=false,dirty=false,loadingProject=false;
    unsigned projectRevision=0;
    TransportState observedTransport;
    Theme theme;
    TransportState state;
    MixerState tracks;
    MidiProject midi;
    AudioOutput audioOutput;
    AudioEditor audioEditor{midi};
    WaveformPreview importer;
    Tick importTime=0;
    struct Import {juce::File file;Tick time;int track;bool sample;};
    std::deque<Import> importQueue;
    int importTrack=4;bool importSample=false;
    void dropItem(const juce::String&,int,Tick,bool);
    unsigned noteRevision=0;
    int mouseNote=-1;
    std::array<bool,128> liveKeys{};
    std::array<double,128> auditionUntil{};
    bool importing=false;
    void nextImport();
    std::vector<std::weak_ptr<const AudioData>> importedMedia;
    bool previewMode=false,audioDirty=true,lastAudioPlaying=false;
    double lastAudioPosition=0,lastAudioTempo=120,lastLoopStart=0,lastLoopEnd=0;
    bool lastLoop=false;
    unsigned audioSeek=0;
    AudioClip auditionClip;
    void previewAudio(const AudioClip&);
    void stopPreview();
    void syncAudio();
    ApplicationMenu menu{state};
    PanelLayout layout;
    Transport transport{state};
    Browser browser;
    Arrangement arrangement{tracks,midi};
    PianoRoll piano{midi};
    std::unique_ptr<ShortcutWindow> shortcuts;
    Mixer mixer{tracks};
    int activeView = 0;
    void showShortcuts();
    DeviceArea devices{tracks};
    Splitter browserDivider{true}, devicesDivider{false};
    juce::TooltipWindow tooltips{this, 600};
    double lastTick = 0, lastMetrics = 0;
    juce::Point<int> lastPointer;
    juce::Component::SafePointer<juce::Component> lastFocus;
    void timerCallback() override;
    void resetLayout();
};
}

