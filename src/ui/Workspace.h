#pragma once
#include "Browser.h"
#include "Transport.h"
#include "Arrangement.h"
#include "ApplicationMenu.h"
#include "Mixer.h"
#include "PianoRoll.h"
#include "ShortcutWindow.h"
namespace auralis
{
class DeviceArea final : public juce::Component
{
public:
    juce::String trackName = "01  Instrument";
    std::uint32_t tint = design::colour::violet;
    void paint(juce::Graphics&) override;
};
class Workspace final : public juce::Component, private juce::Timer
{
public:
    Workspace();
    ~Workspace() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
private:
    Theme theme;
    TransportState state;
    MixerState tracks;
    MidiProject midi;
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
    DeviceArea devices;
    Splitter browserDivider{true}, devicesDivider{false};
    juce::TooltipWindow tooltips{this, 600};
    double lastTick = 0, lastMetrics = 0;
    juce::Point<int> lastPointer;
    juce::Component::SafePointer<juce::Component> lastFocus;
    void timerCallback() override;
    void resetLayout();
};
}
