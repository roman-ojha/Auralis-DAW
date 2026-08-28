#pragma once
#include "Theme.h"
#include "model/SessionView.h"
#include "ContextHelp.h"
#include "MixerControls.h"
#include <vector>
#include "ClipTimeline.h"
#include "AutomationLaneView.h"
namespace auralis
{

class TrackRow final : public juce::Component, public HelpProvider
{
public:
    TrackRow(MixerState&, TrackId);
    void refresh();
    void mouseDoubleClick(const juce::MouseEvent&) override;
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
    std::function<void(int)> onAudioOpen;
    std::function<void(const juce::File&,Tick)> onImport;
    std::function<void(const juce::String&,int,Tick)> onDrop;
    void mouseMove(const juce::MouseEvent&) override;
    std::function<void(Tick,Tick)> onLoop;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    bool keyPressed(const juce::KeyPress&) override;
    void setLoop(Tick start, Tick end, bool enabled);
    void refresh();
    void setKeyboardMode(bool value){clips.computerKeyboardMode=value;}
    void clearRows(){automationRows.clear();rows.clear();}
    void toggleAutomation(){model.automationMode=!model.automationMode;refresh();resized();}
    void focusTimeline() { clips.grabKeyboardFocus(); }
    void createClip() { const auto& c=model.selectedChannel(); clips.createClip(c.type=="MIDI"?c.id:1); }
    void paint(juce::Graphics&) override;
    void resized() override;
    void setPlayhead(double position, int beats);
    void resetLayout();
    void captureView(SessionView& view) const {view.arrangementBar=barWidth;view.trackHeight=rowHeight;view.header=headerWidth;view.vertical=viewport.getViewPositionY();view.arrangementOffset=offsetBars;}
    void restoreView(const SessionView& view){barWidth=view.arrangementBar;rowHeight=view.trackHeight;headerWidth=view.header;offsetBars=view.arrangementOffset;resized();viewport.setViewPosition(0,view.vertical);}

private:
    MidiProject& midi;
    ClipTimeline clips;
    ClipTimeline dropZone;
    Tick loopStart=0, loopEnd=editing::ppq*4, loopAnchor=0;
    bool draggingLoop=false, loopEnabled=false;
    enum class LoopDrag { create, start, end, move } loopDrag=LoopDrag::create;
    Tick originalLoopStart=0,originalLoopEnd=0;
    juce::Component canvas;
    juce::Viewport viewport;
    juce::ScrollBar horizontal{false};
    Splitter trackDivider{true};
    MixerState& model;
    juce::TextButton addTrack,automationToggle;
    std::vector<std::unique_ptr<TrackRow>> rows;
    std::vector<std::unique_ptr<AutomationLaneView>> automationRows;
    int rowHeight = design::trackHeight;
    int headerWidth = design::trackWidth, barWidth = design::defaultBarWidth, beats = 4;
    double offsetBars = 0.0, playhead = 0.0;
    void scrollBarMoved(juce::ScrollBar*, double) override;
    void updateRows();
    void updateScrollRange();
};
}



