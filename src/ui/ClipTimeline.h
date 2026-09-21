#pragma once
#include "Theme.h"
#include "ContextHelp.h"
#include "model/MidiProject.h"
namespace auralis
{
class ClipTimeline final : public juce::Component, public HelpProvider
{
public:
    explicit ClipTimeline(MidiProject&);
    double offsetBars=0;
    int barWidth=100, rowHeight=100;
    Tick selectionStart=0, selectionEnd=0;
    int firstTrack=1,lastTrack=1;
    std::function<void(int)> onOpen;
    std::function<void(Tick,Tick)> onLoop;
    std::function<void(double)> onZoom,onScroll,onVerticalZoom;
    std::function<void(int)> onVerticalScroll;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&,const juce::MouseWheelDetails&) override;
    bool keyPressed(const juce::KeyPress&) override;
    HelpContent helpAt(juce::Point<int>,bool=false) const override;
private:
    MidiProject& model;
    std::vector<MidiClip> clipboard;
    juce::Point<int> anchor;
    Tick anchorTime=0, cursorTime=0;
    int anchorTrack=1,dragId=0;
    MidiClip original;
    enum class Drag {none,select,move,range,resizeLeft,resizeRight,pan} drag=Drag::none;
    Tick at(int x,bool bypass=false) const;
    int trackAt(int y) const;
    int xAt(Tick) const;
    MidiClip* hit(juce::Point<int>);
    juce::Rectangle<int> bounds(const MidiClip&) const;
};
}
