#pragma once
#include "Theme.h"
#include "WaveformPreview.h"
#include <juce_data_structures/juce_data_structures.h>
#include <array>
#include <vector>

namespace auralis
{
class CategoryButton final : public juce::Button
{
public:
    CategoryButton(const juce::String& name, int icon);
    void paintButton(juce::Graphics&, bool, bool) override;
private:
    int icon;
};

class Browser final : public juce::Component, public HelpProvider, private juce::FileBrowserListener
{
public:
    Browser();
    ~Browser() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void resetLayout();
    HelpContent helpAt(juce::Point<int>, bool keyboard = false) const override;
    InfoView infoView;
    struct PluginEntry {juce::String name,description,identity;};
    void setPlugins(std::vector<PluginEntry> list){pluginEntries=std::move(list);if(category==3)filter();}
    void chooseAudio() { waveform.chooseFile(); }
    void setPreviewProgress(double value) { waveform.setProgress(value); }
    std::function<void(std::shared_ptr<const AudioData>)> onPreview;
    std::function<void()> onStopPreview;
private:
    WaveformPreview waveform;
    struct Entry { juce::String name, kind, description, section; };
    std::vector<Entry> entries;
    std::vector<PluginEntry> pluginEntries;
    juce::TextEditor search;
    CategoryButton instruments{"Instruments", 0}, sounds{"Sounds", 1}, effects{"Effects", 2},plugins{"Plug-Ins",3};
    juce::TreeView tree;
    std::unique_ptr<juce::TreeViewItem> root;
    Splitter divider{true};
    juce::Rectangle<int> resultsBounds;
    int category = 0, navigationWidth = design::browser::navigationWidth, matchCount = 0;
    juce::String selection;
    juce::TextButton addLibrary,removeLibrary;
    juce::Viewport categoryViewport;
    juce::Component categoryList;
    std::vector<std::unique_ptr<juce::TextButton>> folderButtons;
    juce::StringArray folders;
    std::unique_ptr<juce::FileChooser> folderChooser;
    std::unique_ptr<juce::PropertiesFile> preferences;
    juce::TimeSliceThread directoryThread{"Library folders"};
    struct LibraryFilter : juce::FileFilter
    {
        LibraryFilter():FileFilter("Audio files"){}
        mutable std::mutex mutex;
        juce::String query;
        bool isFileSuitable(const juce::File& file) const override
        {
            const std::lock_guard lock(mutex);
            return file.hasFileExtension("wav;aif;aiff;flac")&&file.getFileName().containsIgnoreCase(query);
        }
        bool isDirectorySuitable(const juce::File&) const override { return true; }
    } audioFilter;
    juce::DirectoryContentsList directory{&audioFilter,directoryThread};
    juce::FileTreeComponent files{directory};
    std::shared_ptr<const AudioData> previewMedia;
    void addFolder();
    void rebuildFolders();
    void selectionChanged() override;
    void fileClicked(const juce::File&,const juce::MouseEvent&) override;
    void fileDoubleClicked(const juce::File&) override {}
    void browserRootChanged(const juce::File&) override {}
    void filter();
    void selectCategory(int);
};
}

