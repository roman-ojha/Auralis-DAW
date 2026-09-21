#pragma once
#include "ContextHelp.h"
#include "model/MixerState.h"

namespace auralis
{
enum class ControlIcon { arrangement, mixer, piano, play, pause, stop, record, power, polarity, route, dock, add };
class IconButton : public juce::Button
{
public:
    IconButton(ControlIcon icon, const juce::String& title, const juce::String& help);
    void paintButton(juce::Graphics&, bool, bool) override;
    ControlIcon icon;
    bool solo = false, muted = false;
};
class MuteSoloButton final : public IconButton
{
public:
    MuteSoloButton(MixerState&, TrackId);
    void refresh();
    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    bool keyPressed(const juce::KeyPress&) override;
private:
    MixerState& state;
    TrackId id;
    bool soloGesture = false;
};
class Knob final : public juce::Slider
{
public:
    Knob(const juce::String& title, const juce::String& help, double minimum, double maximum, double initial,
         std::uint32_t tint = design::colour::mint);
};
void drawSilentMeter(juce::Graphics&, juce::Rectangle<int>, std::uint32_t tint, bool scale = false);
}
