#pragma once
#include "Theme.h"
#include "WaveformPreview.h"
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

class Browser final : public juce::Component, public HelpProvider
{
public:
    Browser();
    ~Browser() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void resetLayout();
    HelpContent helpAt(juce::Point<int>, bool keyboard = false) const override;
    InfoView infoView;
    void chooseAudio() { waveform.chooseFile(); }
private:
    WaveformPreview waveform;
    struct Entry { juce::String name, kind, description, section; };
    std::vector<Entry> entries;
    juce::TextEditor search;
    CategoryButton instruments{"Instruments", 0}, sounds{"Sounds", 1}, effects{"Effects", 2};
    juce::TreeView tree;
    std::unique_ptr<juce::TreeViewItem> root;
    Splitter divider{true};
    juce::Rectangle<int> resultsBounds;
    int category = 0, navigationWidth = design::browser::navigationWidth, matchCount = 0;
    juce::String selection;
    void filter();
    void selectCategory(int);
};
}

