#pragma once
#include "Theme.h"
#include "ContextHelp.h"
#include "MixerControls.h"
#include <vector>
#include "ClipTimeline.h"
namespace auralis
{

class TrackRow final : public juce::Component, public HelpProvider
{
public:
    TrackRow(MixerState&, TrackId);
    void refresh();
    ChannelState& info;
    bool selected = false;
    int headerWidth = design::trackWidth;
    double offsetBars = 0, playheadBars = 0;
    int barWidth = design::defaultBarWidth, beatsPerBar = 4;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    HelpContent helpAt(juce::Point<int>, bool keyboard = false) const override;
private:
    MixerState& model;
    TrackId index;
    juce::TextButton nameButton, arm;
    MuteSoloButton mute;
    Knob gain;
};
class Arrangement final : public juce::Component, private juce::ScrollBar::Listener
{
public:
    Arrangement(MixerState&, MidiProject&);
    std::function<void(int)> onOpen;
    std::function<void(Tick,Tick)> onLoop;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void setLoop(Tick start, Tick end, bool enabled);
    void refresh();
    void focusTimeline() { clips.grabKeyboardFocus(); }
    void createClip() { clips.keyPressed(juce::KeyPress('M',juce::ModifierKeys::ctrlModifier|juce::ModifierKeys::shiftModifier,0)); }
    void paint(juce::Graphics&) override;
    void resized() override;
    void setPlayhead(double position, int beats);
    void resetLayout();
private:
    MidiProject& midi;
    ClipTimeline clips;
    Tick loopStart=0, loopEnd=editing::ppq*4, loopAnchor=0;
    bool draggingLoop=false, loopEnabled=false;
    juce::Component canvas;
    juce::Viewport viewport;
    juce::ScrollBar horizontal{false};
    Splitter trackDivider{true};
    MixerState& model;
    juce::TextButton addTrack;
    std::vector<std::unique_ptr<TrackRow>> rows;
    int rowHeight = design::trackHeight;
    int headerWidth = design::trackWidth, barWidth = design::defaultBarWidth, beats = 4;
    double offsetBars = 0.0, playhead = 0.0;
    void scrollBarMoved(juce::ScrollBar*, double) override;
    void updateRows();
    void updateScrollRange();
};
}


