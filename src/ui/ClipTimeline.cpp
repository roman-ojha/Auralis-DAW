#include "ClipTimeline.h"
namespace auralis
{
ClipTimeline::ClipTimeline(MidiProject& m):model(m){setWantsKeyboardFocus(true);}
Tick ClipTimeline::at(int x,bool bypass) const{return model.snap(static_cast<Tick>((offsetBars+static_cast<double>(x)/barWidth)*model.barTicks),bypass);}
int ClipTimeline::trackAt(int y) const{return juce::jlimit(1,4,y/rowHeight+1);}
int ClipTimeline::xAt(Tick tick) const{return juce::roundToInt((static_cast<double>(tick)/model.barTicks-offsetBars)*barWidth);}
juce::Rectangle<int> ClipTimeline::bounds(const MidiClip& c) const{return {xAt(c.start),(c.track-1)*rowHeight+3,juce::jmax(3,xAt(c.start+c.length)-xAt(c.start)),rowHeight-6};}
MidiClip* ClipTimeline::hit(juce::Point<int> p){for(auto it=model.clips.rbegin();it!=model.clips.rend();++it)if(bounds(*it).contains(p))return &*it;return nullptr;}
HelpContent ClipTimeline::helpAt(juce::Point<int>,bool) const
{
    return {"Arrangement MIDI clips", "Double-right-click an instrument lane to create a clip at the grid. Double-left-click opens piano roll. Drag a clip header to move, edges to trim/repeat. Drag the body or empty lane (or Ctrl+right-drag) to select time across tracks. Drag inside a selection to move its independent fragments. Delete cuts the range; Ctrl+E splits at cursor; Ctrl+L loops selection. Ctrl+Z undo. F1: shortcuts."};
}
void ClipTimeline::paint(juce::Graphics& g)
{
    // Transparent timeline overlay retains track background/grid and meters underneath.
    for(const auto& c:model.clips)
    {
        const auto r=bounds(c);if(!r.intersects(getLocalBounds()))continue;
        const auto tint=colour(c.track==1?design::colour::violet:c.track==2?design::colour::mint:design::colour::amber);
        g.setColour(tint.withAlpha(0.3f));g.fillRoundedRectangle(r.toFloat(),3);
        g.setColour(tint.withAlpha(c.id==model.active?0.85f:0.6f));g.fillRect(r.withHeight(20));
        text(g,juce::String(c.name),r.withHeight(20).reduced(5,0),10,design::colour::background,true);
        {juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(r.withTrimmedTop(21));
        for(const auto& n:MidiProject::renderedNotes(c))
        {
            const int y=r.getY()+24+(127-n.pitch)*(r.getHeight()-28)/128;
            g.setColour(tint.withAlpha(n.muted?0.25f:1));g.fillRect(xAt(c.start+n.start),y,juce::jmax(2,xAt(c.start+n.start+n.length)-xAt(c.start+n.start)),2);
        }
        for(Tick repeat=c.cycle-c.offset;repeat<c.length;repeat+=c.cycle)
        {g.setColour(tint.withAlpha(0.3f));g.drawVerticalLine(xAt(c.start+repeat),static_cast<float>(r.getY()+20),static_cast<float>(r.getBottom()));}}
        g.setColour(tint);g.drawRoundedRectangle(r.toFloat(),3,c.id==model.active?1.5f:0.6f);
    }
    if(selectionEnd>selectionStart)
    {
        const juce::Rectangle<int> r{xAt(selectionStart),(firstTrack-1)*rowHeight,xAt(selectionEnd)-xAt(selectionStart),(lastTrack-firstTrack+1)*rowHeight};
        g.setColour(colour(design::colour::mint).withAlpha(0.2f));g.fillRect(r);g.setColour(colour(design::colour::mint));g.drawRect(r);
    }
}
void ClipTimeline::mouseDown(const juce::MouseEvent& e)
{
    grabKeyboardFocus();anchor=e.getPosition();anchorTime=cursorTime=at(e.x,e.mods.isAltDown());anchorTrack=trackAt(e.y);
    if(e.mods.isMiddleButtonDown()){drag=Drag::pan;return;}
    if(e.mods.isRightButtonDown()&&!e.mods.isCtrlDown())return;
    if(!e.mods.isCtrlDown()&&selectionEnd>selectionStart&&anchorTime>=selectionStart&&anchorTime<selectionEnd&&anchorTrack>=firstTrack&&anchorTrack<=lastTrack)
    {drag=Drag::range;return;}
    if(auto* c=hit(anchor);c&&!e.mods.isCtrlDown())
    {
        model.active=c->id;original=*c;dragId=c->id;const auto r=bounds(*c);
        if(e.x<=r.getX()+5)drag=Drag::resizeLeft;
        else if(e.x>=r.getRight()-5)drag=Drag::resizeRight;
        else if(e.y<r.getY()+20)drag=Drag::move;
        else drag=Drag::select;
        if(drag!=Drag::select)model.checkpoint();model.changed();
    }
    else drag=Drag::select;
    selectionStart=selectionEnd=anchorTime;firstTrack=lastTrack=anchorTrack;repaint();
}
void ClipTimeline::mouseDoubleClick(const juce::MouseEvent& e)
{
    drag=Drag::none;
    if(e.mods.isRightButtonDown()&&!e.mods.isCtrlDown())
    {
        if(!hit(e.getPosition())){const int id=model.create(trackAt(e.y),at(e.x,e.mods.isAltDown()));if(id&&onOpen)onOpen(id);}
    }
    else if(auto* c=hit(e.getPosition())){model.active=c->id;if(onOpen)onOpen(c->id);}
}
void ClipTimeline::mouseDrag(const juce::MouseEvent& e)
{
    if(drag==Drag::pan){if(onScroll)onScroll(static_cast<double>(anchor.x-e.x)/barWidth);if(onVerticalScroll)onVerticalScroll(anchor.y-e.y);anchor=e.getPosition();return;}
    const Tick time=at(e.x,e.mods.isAltDown());
    if(drag==Drag::select)
    {selectionStart=std::min(anchorTime,time);selectionEnd=std::max(anchorTime,time);firstTrack=std::min(anchorTrack,trackAt(e.y));lastTrack=std::max(anchorTrack,trackAt(e.y));repaint();return;}
    if(auto* c=model.clip(dragId))
    {
        if(drag==Drag::move){c->start=std::clamp<Tick>(original.start+time-anchorTime,0,editing::maximumTime-c->length);c->track=juce::jlimit(1,3,original.track+trackAt(e.y)-anchorTrack);}
        if(drag==Drag::resizeRight)c->length=std::clamp<Tick>(original.length+time-anchorTime,editing::minimumNote,editing::maximumTime-c->start);
        if(drag==Drag::resizeLeft)
        {
            const Tick start=std::clamp<Tick>(original.start+time-anchorTime,0,original.start+original.length-editing::minimumNote);
            c->start=start;c->length=original.start+original.length-start;c->offset=((original.offset+start-original.start)%c->cycle+c->cycle)%c->cycle;
        }
        model.changed();
    }
}
void ClipTimeline::mouseUp(const juce::MouseEvent& e)
{
    if(drag==Drag::range&&e.getDistanceFromDragStart()>3)
    {
        model.checkpoint();const Tick delta=std::clamp<Tick>(at(e.x,e.mods.isAltDown())-anchorTime,-selectionStart,editing::maximumTime-selectionEnd);
        const int last=std::min(lastTrack,3), first=std::min(firstTrack,3);
        const int trackDelta=juce::jlimit(1-first,3-last,trackAt(e.y)-anchorTrack);
        model.moveRange(selectionStart,selectionEnd,first,last,delta,trackDelta);selectionStart+=delta;selectionEnd+=delta;firstTrack=first+trackDelta;lastTrack=last+trackDelta;model.changed();
    }
    drag=Drag::none;
}
void ClipTimeline::mouseWheelMove(const juce::MouseEvent& e,const juce::MouseWheelDetails& w)
{
    if(e.mods.isCtrlDown()&&e.mods.isAltDown()){if(onVerticalZoom)onVerticalZoom(w.deltaY>0?1.1:1/1.1);}
    else if(e.mods.isCtrlDown()){if(onZoom)onZoom(w.deltaY>0?1.2:1/1.2);}
    else if(e.mods.isShiftDown()||std::abs(w.deltaX)>std::abs(w.deltaY)){if(onScroll)onScroll(-(e.mods.isShiftDown()?w.deltaY:w.deltaX)*4);}
    else if(onVerticalScroll)onVerticalScroll(juce::roundToInt(-w.deltaY*200));
}
bool ClipTimeline::keyPressed(const juce::KeyPress& key)
{
    juce::File::getSpecialLocation(juce::File::currentExecutableFile).getSiblingFile("key-debug.txt").appendText("ClipTimeline " + juce::String(key.getKeyCode())+" mods="+juce::String(key.getModifiers().getRawFlags())+" text="+juce::String(static_cast<int>(key.getTextCharacter()))+"\n"); // TEMP_KEY_DIAGNOSTIC
    const int rawCode=key.getKeyCode();const int code=(rawCode>='a'&&rawCode<='z')?rawCode-'a'+'A':rawCode;const bool ctrl=key.getModifiers().isCtrlDown();
    if(code==juce::KeyPress::pageUpKey||code==juce::KeyPress::pageDownKey){if(onZoom)onZoom(code==juce::KeyPress::pageUpKey?1.25:0.8);return true;}
    if(code==juce::KeyPress::backspaceKey){model.snapIndex=model.snapIndex==0?1:0;model.changed();return true;}
    if(ctrl&&key.getModifiers().isShiftDown()&&code=='M'){const int id=model.create(anchorTrack,cursorTime);if(id&&onOpen)onOpen(id);return true;}
    if(ctrl&&code=='L'){if(selectionEnd>selectionStart&&onLoop)onLoop(selectionStart,selectionEnd);return true;}
    if(ctrl&&code=='A'){selectionStart=0;selectionEnd=model.barTicks;for(const auto& c:model.clips)selectionEnd=std::max(selectionEnd,c.start+c.length);firstTrack=1;lastTrack=3;repaint();return true;}
    if(ctrl&&code=='D'){selectionStart=selectionEnd=0;repaint();return true;}
    if(ctrl&&code=='E')
    {
        model.checkpoint();std::vector<MidiClip> result;
        for(const auto& c:model.clips)if(c.track==anchorTrack&&cursorTime>c.start&&cursorTime<c.start+c.length){result.push_back(model.slice(c,c.start,cursorTime));result.push_back(model.slice(c,cursorTime,c.start+c.length));}else result.push_back(c);
        model.clips=std::move(result);model.active=0;model.changed();return true;
    }
    if(ctrl&&(code=='C'||code=='X'||code=='B'))
    {
        clipboard.clear();for(const auto& c:model.clips)
        {
            if(selectionEnd>selectionStart){if(c.track>=firstTrack&&c.track<=lastTrack&&c.start<selectionEnd&&c.start+c.length>selectionStart)clipboard.push_back(model.slice(c,std::max(c.start,selectionStart),std::min(c.start+c.length,selectionEnd)));}
            else if(c.id==model.active)clipboard.push_back(c);
        }
        if(code=='C')return true;
    }
    if(code==juce::KeyPress::deleteKey||(ctrl&&code=='X'))
    {
        model.checkpoint();if(selectionEnd>selectionStart)model.deleteRange(selectionStart,selectionEnd,firstTrack,lastTrack);else{std::erase_if(model.clips,[this](const auto& c){return c.id==model.active;});model.active=0;}model.changed();return true;
    }
    if(ctrl&&(code=='V'||code=='B'))
    {
        if(clipboard.empty())return true;model.checkpoint();Tick start=clipboard.front().start,end=0;
        for(const auto& c:clipboard){start=std::min(start,c.start);end=std::max(end,c.start+c.length);}
        const Tick delta=(code=='B'?end:cursorTime)-start;
        for(auto c:clipboard)if(c.start+delta>=0&&c.start+delta+c.length<=editing::maximumTime&&model.clips.size()<editing::maximumClips){c.id=model.freshClipId();c.start+=delta;model.clips.push_back(c);model.active=c.id;}
        selectionStart=selectionEnd=0;model.changed();return true;
    }
    return false;
}
}
