#pragma once
#include "Theme.h"
#include "ContextHelp.h"
#include "model/MidiProject.h"
namespace auralis
{
class ClipTimeline final : public juce::Component, public HelpProvider, public juce::DragAndDropTarget, public juce::FileDragAndDropTarget
{
public:
    explicit ClipTimeline(MidiProject&);
    double offsetBars=0;
    int barWidth=100, rowHeight=100;
    bool showDropHint=true,computerKeyboardMode=false;
    Tick selectionStart=0, selectionEnd=0;
    int firstTrack=1,lastTrack=1;
    std::vector<int> laneIds{1,2,3,4};
    std::vector<int> laneTops;
    int top(int id) const {const int index=lane(id);return index<0?-rowHeight:laneTops.size()==laneIds.size()?laneTops[index]:index*rowHeight;}
    std::function<void(int)> onTrackSelect;
    std::function<void(const juce::String&,int,Tick)> onDrop;
    std::function<void(int)> onOpen;
    std::function<void(int)> onAudioOpen;
    std::function<void(const juce::File&,Tick)> onImport;
    bool isInterestedInDragSource(const SourceDetails& d) override { return d.description.toString().startsWith("device:")||d.description.toString().startsWith("plugin:")||juce::File(d.description.toString()).hasFileExtension("wav;aif;aiff;flac"); }
    void itemDropped(const SourceDetails& d) override { if(onDrop)onDrop(d.description.toString(),trackAt(d.localPosition.y),at(d.localPosition.x)); }
    bool isInterestedInFileDrag(const juce::StringArray& files) override { return files.size()==1&&juce::File(files[0]).hasFileExtension("wav;aif;aiff;flac"); }
    void filesDropped(const juce::StringArray& files,int x,int y) override { if(files.size()==1&&onDrop)onDrop(files[0],trackAt(y),at(x)); }
    std::function<void(Tick,Tick)> onLoop;
    std::function<void(Tick,Tick)> onZoomRange;
    std::function<void(double)> onZoom,onScroll,onVerticalZoom;
    std::function<void(int)> onVerticalScroll;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override { setMouseCursor(juce::MouseCursor::NormalCursor); }
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&,const juce::MouseWheelDetails&) override;
    bool keyPressed(const juce::KeyPress&) override;
    HelpContent helpAt(juce::Point<int>,bool=false) const override;
    void createClip(int track = 0);
private:
    MidiProject& model;
    std::vector<MidiClip> clipboard;
    juce::Point<int> anchor;
    Tick anchorTime=0, cursorTime=0;
    int anchorTrack=1,dragId=0;
    MidiClip original;
    AudioClip originalAudio;
    std::vector<AudioClip> audioClipboard;
    bool draggingAudio=false;
    enum class Drag {none,select,move,range,resizeLeft,resizeRight,pan,fadeIn,fadeOut} drag=Drag::none;
    Tick at(int x,bool bypass=false) const;
    int trackAt(int y) const;
    int lane(int id) const { const auto i=std::find(laneIds.begin(),laneIds.end(),id);return i==laneIds.end()?-1:static_cast<int>(i-laneIds.begin()); }
    bool instrument(int id) const { return std::find(model.instrumentTracks.begin(),model.instrumentTracks.end(),id)!=model.instrumentTracks.end(); }
    int xAt(Tick) const;
    MidiClip* hit(juce::Point<int>);
    juce::Rectangle<int> bounds(const MidiClip&) const;
    juce::Rectangle<int> bounds(const AudioClip& c) const { return {xAt(c.start),top(c.track)+3,juce::jmax(3,xAt(c.start+c.length)-xAt(c.start)),rowHeight-6}; }
    AudioClip* audioHit(juce::Point<int>);
    void paintAudio(juce::Graphics&);
};
}

