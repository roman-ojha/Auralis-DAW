#include "ClipTimeline.h"
#include "EditorKeys.h"
namespace auralis
{
ClipTimeline::ClipTimeline(MidiProject& m):model(m){setWantsKeyboardFocus(true);}
void ClipTimeline::createClip(int track)
{
    const int id=model.create(track==0?anchorTrack:track,cursorTime);
    if(id&&onOpen)onOpen(id);
}
Tick ClipTimeline::at(int x,bool bypass) const{return model.snap(static_cast<Tick>((offsetBars+static_cast<double>(x)/barWidth)*model.barTicks),bypass,static_cast<double>(barWidth)*editing::ppq/model.barTicks);}
int ClipTimeline::trackAt(int y) const{for(int id:laneIds)if(y>=top(id)&&y<top(id)+rowHeight)return id;return -1;}
int ClipTimeline::xAt(Tick tick) const{return juce::roundToInt((static_cast<double>(tick)/model.barTicks-offsetBars)*barWidth);}
juce::Rectangle<int> ClipTimeline::bounds(const MidiClip& c) const{return {xAt(c.start),top(c.track)+3,juce::jmax(3,xAt(c.start+c.length)-xAt(c.start)),rowHeight-6};}
MidiClip* ClipTimeline::hit(juce::Point<int> p){for(auto it=model.clips.rbegin();it!=model.clips.rend();++it)if(bounds(*it).contains(p))return &*it;return nullptr;}
AudioClip* ClipTimeline::audioHit(juce::Point<int> p){for(auto it=model.audioClips.rbegin();it!=model.audioClips.rend();++it)if(bounds(*it).contains(p))return &*it;return nullptr;}
void ClipTimeline::paintAudio(juce::Graphics& g)
{
    for(const auto& c:model.audioClips)
    {
        auto r=bounds(c);if(!r.intersects(getLocalBounds())||!c.source)continue;
        const auto tint=colour(design::colour::mint);
        g.setColour(tint.withAlpha(c.muted?0.1f:0.25f));g.fillRoundedRectangle(r.toFloat(),3);
        g.setColour(tint);g.fillRect(r.withHeight(20));
        text(g,juce::String(c.name),r.withHeight(20).reduced(5,0),10,design::colour::background,true);
        const double duration=c.length*60.0/(model.tempo*editing::ppq);
        const auto wave=r.withTrimmedTop(24).reduced(0,4);
        for(int x=std::max(0,r.getX());x<std::min(getWidth(),r.getRight());++x)
        {
            const double elapsed=static_cast<double>(x-r.getX())/r.getWidth()*duration;
            double local=c.phase+elapsed*c.speed(),region=c.sourceEnd-c.sourceStart;
            if(region<=0)continue;
            if(c.loop)local=std::fmod(local,region);
            if(local>=region)continue;
            double source=c.reverse?c.sourceEnd-local:c.sourceStart+local;
            const int bin=std::clamp(static_cast<int>(source/c.source->seconds()*audio::peaks),0,audio::peaks-1);
            float amplitude=c.source->peaks[static_cast<size_t>(bin)]*wave.getHeight()/2;
            g.setColour(tint.withAlpha(c.muted?0.2f:0.9f));g.drawVerticalLine(x,wave.getCentreY()-amplitude,wave.getCentreY()+amplitude);
        }
        const int inX=r.getX()+juce::roundToInt(c.fadeIn/duration*r.getWidth());
        const int outX=r.getRight()-juce::roundToInt(c.fadeOut/duration*r.getWidth());
        g.setColour(colour(design::colour::amber));
        g.drawLine(static_cast<float>(r.getX()),static_cast<float>(r.getBottom()),static_cast<float>(inX),static_cast<float>(r.getY()+23),2);
        g.drawLine(static_cast<float>(outX),static_cast<float>(r.getY()+23),static_cast<float>(r.getRight()),static_cast<float>(r.getBottom()),2);
        g.fillRect(inX-4,r.getY()+20,8,8);g.fillRect(outX-4,r.getY()+20,8,8);
        g.setColour(c.id==model.activeAudio?colour(design::colour::amber):tint);g.drawRect(r,c.id==model.activeAudio?2:1);
    }
}
HelpContent ClipTimeline::helpAt(juce::Point<int> point,bool) const
{
    if(!instrument(trackAt(point.y)))return {"Arrangement audio clips","Drag a library WAV, AIFF or FLAC here to import it. Click a clip to open its editor below. Drag the header to move, edges to trim/extend, amber handles just below the header for fades. Amber regions show selected clip portions. Ctrl+B duplicates independently; Ctrl+E splits; Delete cuts selected time; Ctrl+Z undoes. Hover cursors show move/resize/select. Audio clip limit: 128."};
    return {"Arrangement MIDI clips", "Double-click an empty instrument lane with either mouse button to create a clip at the grid. Double-left-click opens piano roll. Drag a clip header to move, edges to trim/repeat. Drag the body or empty lane (or Ctrl+right-drag) to select time across tracks. Drag inside a selection to move its independent fragments. Delete cuts the range; Ctrl+E splits at cursor; Ctrl+L loops selection. Ctrl+Z undo. F1: shortcuts."};
}
void ClipTimeline::paint(juce::Graphics& g)
{
    if(showDropHint)text(g,"Drag clip/instrument here",{8,static_cast<int>(laneIds.size())*rowHeight+12,getWidth()-16,30},13,design::colour::muted,false,juce::Justification::centred);
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
    paintAudio(g);
    if(selectionEnd>selectionStart)
    {
        const juce::Rectangle<int> r{xAt(selectionStart),top(firstTrack),xAt(selectionEnd)-xAt(selectionStart),top(lastTrack)+rowHeight-top(firstTrack)};
        g.setColour(colour(design::colour::mint).withAlpha(0.09f));g.fillRect(r);g.setColour(colour(design::colour::mint));g.drawRect(r);
        int count=0;
        for(const auto& c:model.clips)
            if(c.track>=firstTrack&&c.track<=lastTrack&&c.start<selectionEnd&&c.start+c.length>selectionStart)
            {
                const auto selected=bounds(c).getIntersection(r);
                g.setColour(colour(design::colour::amber).withAlpha(0.3f));g.fillRect(selected);
                g.setColour(colour(design::colour::amber));g.drawRect(selected,2);++count;
            }
        for(const auto& c:model.audioClips)
            if(c.track>=firstTrack&&c.track<=lastTrack&&c.start<selectionEnd&&c.start+c.length>selectionStart)
            {
                const auto selected=bounds(c).getIntersection(r);
                g.setColour(colour(design::colour::amber).withAlpha(0.3f));g.fillRect(selected);
                g.setColour(colour(design::colour::amber));g.drawRect(selected,2);++count;
            }
        text(g,juce::String(count)+" clips",r.withHeight(20).reduced(5,0),11,design::colour::text,true);
    }
}
void ClipTimeline::mouseMove(const juce::MouseEvent& e)
{
    auto pointerStyle=juce::MouseCursor::CrosshairCursor;
    const auto time=at(e.x,true);
    if(selectionEnd>selectionStart&&time>=selectionStart&&time<selectionEnd&&trackAt(e.y)>=firstTrack&&trackAt(e.y)<=lastTrack&&!e.mods.isCtrlDown())
        pointerStyle=juce::MouseCursor::DraggingHandCursor;
    else if(auto* c=hit(e.getPosition());c&&!e.mods.isCtrlDown())
    {
        const auto r=bounds(*c);
        if(e.x<=r.getX()+5||e.x>=r.getRight()-5)pointerStyle=juce::MouseCursor::LeftRightResizeCursor;
        else if(e.y<r.getY()+20)pointerStyle=juce::MouseCursor::DraggingHandCursor;
    }
    else if(auto* audio=audioHit(e.getPosition());audio&&!e.mods.isCtrlDown())
    {
        const auto r=bounds(*audio);
        if(e.y>=r.getY()+20&&e.y<r.getY()+32)pointerStyle=juce::MouseCursor::LeftRightResizeCursor;
        else if(e.x<=r.getX()+5||e.x>=r.getRight()-5)pointerStyle=juce::MouseCursor::LeftRightResizeCursor;
        else if(e.y<r.getY()+20)pointerStyle=juce::MouseCursor::DraggingHandCursor;
    }
    setMouseCursor(pointerStyle);
}
void ClipTimeline::mouseDown(const juce::MouseEvent& e)
{
    if(trackAt(e.y)<0)return;
    grabKeyboardFocus();draggingAudio=false;anchor=e.getPosition();anchorTime=cursorTime=at(e.x,e.mods.isAltDown());anchorTrack=trackAt(e.y);
    if(e.mods.isMiddleButtonDown()){drag=Drag::pan;return;}
    if(e.mods.isRightButtonDown()&&!e.mods.isCtrlDown())return;
    if(!e.mods.isCtrlDown()&&selectionEnd>selectionStart&&anchorTime>=selectionStart&&anchorTime<selectionEnd&&anchorTrack>=firstTrack&&anchorTrack<=lastTrack)
    {drag=Drag::range;return;}
    if(auto* c=audioHit(anchor);c&&!e.mods.isCtrlDown())
    {
        model.activeAudio=c->id;model.active=0;originalAudio=*c;dragId=c->id;draggingAudio=true;
        const auto r=bounds(*c);const double duration=c->length*60.0/(model.tempo*editing::ppq);
        const int inX=r.getX()+juce::roundToInt(c->fadeIn/duration*r.getWidth()),outX=r.getRight()-juce::roundToInt(c->fadeOut/duration*r.getWidth());
        if(e.y>=r.getY()+20&&e.y<r.getY()+32&&std::abs(e.x-inX)<=8)drag=Drag::fadeIn;
        else if(e.y>=r.getY()+20&&e.y<r.getY()+32&&std::abs(e.x-outX)<=8)drag=Drag::fadeOut;
        else if(e.x<=r.getX()+5)drag=Drag::resizeLeft;
        else if(e.x>=r.getRight()-5)drag=Drag::resizeRight;
        else if(e.y<r.getY()+20)drag=Drag::move;
        else {drag=Drag::select;if(onTrackSelect)onTrackSelect(anchorTrack);}
        if(drag!=Drag::select)model.checkpoint();
        selectionStart=selectionEnd=anchorTime;firstTrack=lastTrack=c->track;
        if(onAudioOpen)onAudioOpen(c->id);model.changed();return;
    }
    if(auto* c=hit(anchor);c&&!e.mods.isCtrlDown())
    {
        model.activeAudio=0;model.active=c->id;original=*c;dragId=c->id;const auto r=bounds(*c);
        if(e.x<=r.getX()+5)drag=Drag::resizeLeft;
        else if(e.x>=r.getRight()-5)drag=Drag::resizeRight;
        else if(e.y<r.getY()+20)drag=Drag::move;
        else {drag=Drag::select;if(onTrackSelect)onTrackSelect(anchorTrack);}
        if(drag!=Drag::select)model.checkpoint();model.changed();
    }
    else {drag=Drag::select;if(onTrackSelect)onTrackSelect(anchorTrack);}
    selectionStart=selectionEnd=anchorTime;firstTrack=lastTrack=anchorTrack;repaint();
}
void ClipTimeline::mouseDoubleClick(const juce::MouseEvent& e)
{
    drag=Drag::none;
    if(e.mods.isCtrlDown() || e.mods.isMiddleButtonDown()) return;
    if(auto* c=audioHit(e.getPosition())){model.activeAudio=c->id;if(onAudioOpen)onAudioOpen(c->id);return;}
    if(auto* c=hit(e.getPosition()))
    {
        model.active=c->id;
        if(onOpen)onOpen(c->id);
    }
    else
    {
        selectionStart=selectionEnd=0;
        const int id=model.create(trackAt(e.y),at(e.x,e.mods.isAltDown()));
        if(id&&onOpen)onOpen(id);
    }
}
void ClipTimeline::mouseDrag(const juce::MouseEvent& e)
{
    if(drag==Drag::pan){if(onScroll)onScroll(static_cast<double>(anchor.x-e.x)/barWidth);if(onVerticalScroll)onVerticalScroll(anchor.y-e.y);anchor=e.getPosition();return;}
    const Tick time=at(e.x,e.mods.isAltDown());
    if(drag==Drag::select)
    {const int target=trackAt(e.y)>0?trackAt(e.y):(e.y<0?laneIds.front():laneIds.back());selectionStart=std::min(anchorTime,time);selectionEnd=std::max(anchorTime,time);firstTrack=std::min(anchorTrack,target);lastTrack=std::max(anchorTrack,target);repaint();return;}
    if(draggingAudio)
    {
        if(auto* c=model.audioClip(dragId))
        {
            const double secondsPerTick=60.0/(model.tempo*editing::ppq);
            if(drag==Drag::move){c->start=std::clamp<Tick>(originalAudio.start+time-anchorTime,0,editing::maximumTime-c->length);if(trackAt(e.y)>0&&!instrument(trackAt(e.y)))c->track=trackAt(e.y);}
            if(drag==Drag::resizeRight)c->length=std::clamp<Tick>(originalAudio.length+time-anchorTime,editing::minimumNote,editing::maximumTime-c->start);
            if(drag==Drag::resizeLeft)
            {
                const Tick minimum=std::max<Tick>(0,originalAudio.start-static_cast<Tick>(originalAudio.phase/originalAudio.speed()/secondsPerTick));
                c->start=std::clamp<Tick>(originalAudio.start+time-anchorTime,minimum,originalAudio.start+originalAudio.length-editing::minimumNote);
                c->length=originalAudio.start+originalAudio.length-c->start;
                c->phase=originalAudio.phase+(c->start-originalAudio.start)*secondsPerTick*c->speed();
            }
            if(drag==Drag::fadeIn)c->fadeIn=std::clamp((at(e.x,true)-c->start)*secondsPerTick,0.0,c->length*secondsPerTick);
            if(drag==Drag::fadeOut)c->fadeOut=std::clamp((c->start+c->length-at(e.x,true))*secondsPerTick,0.0,c->length*secondsPerTick);
            model.changed();
        }
        return;
    }
    if(auto* c=model.clip(dragId))
    {
        if(drag==Drag::move){c->start=std::clamp<Tick>(original.start+time-anchorTime,0,editing::maximumTime-c->length);if(instrument(trackAt(e.y)))c->track=trackAt(e.y);}
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
        const int last=lastTrack, first=firstTrack;
        const int trackDelta=0; // Mixed-type range moves preserve lanes; individual clip headers move across compatible tracks.
        model.moveRange(selectionStart,selectionEnd,first,last,delta,trackDelta);selectionStart+=delta;selectionEnd+=delta;firstTrack=first+trackDelta;lastTrack=last+trackDelta;model.changed();
    }
    drag=Drag::none;
}
void ClipTimeline::mouseWheelMove(const juce::MouseEvent& e,const juce::MouseWheelDetails& w)
{
    if(e.mods.isAltDown()){if(onVerticalZoom)onVerticalZoom(w.deltaY>0?1.1:1/1.1);}
    else if(e.mods.isCtrlDown()){if(onZoom)onZoom(w.deltaY>0?1.2:1/1.2);}
    else if(e.mods.isShiftDown()||std::abs(w.deltaX)>std::abs(w.deltaY)){if(onScroll)onScroll(-(e.mods.isShiftDown()?w.deltaY:w.deltaX)*4);}
    else if(onVerticalScroll)onVerticalScroll(juce::roundToInt(-w.deltaY*200));
}
bool ClipTimeline::keyPressed(const juce::KeyPress& key)
{
    if(computerKeyboardMode&&!key.getModifiers().isAnyModifierKeyDown()&&key.getKeyCode()>=65&&key.getKeyCode()<=90)return false;
    const EditorKey input(key);const int code=input.code;const bool ctrl=input.control;
    if(!ctrl&&key.getModifiers().isShiftDown()&&code>='1'&&code<='3')
    {
        if(onZoom)onZoom(static_cast<double>(design::defaultBarWidth*(1<<(code-'1')))/barWidth);
        return true;
    }
    if(!ctrl&&(code=='Z'||(key.getModifiers().isShiftDown()&&(code=='4'||code=='5'))))
    {
        Tick start=0,end=model.barTicks;
        if(code!='4'&&selectionEnd>selectionStart){start=selectionStart;end=selectionEnd;}
        else if(code!='4'&&model.clip(model.active)){start=model.clip(model.active)->start;end=start+model.clip(model.active)->length;}
        else
        {
            for(const auto& c:model.clips)end=std::max(end,c.start+c.length);
            for(const auto& c:model.audioClips)end=std::max(end,c.start+c.length);
        }
        if(onZoomRange)onZoomRange(start,end);return true;
    }
    if(code==juce::KeyPress::pageUpKey||code==juce::KeyPress::pageDownKey){if(onZoom)onZoom(code==juce::KeyPress::pageUpKey?1.25:0.8);return true;}
    if(code==juce::KeyPress::backspaceKey){model.snapIndex=model.snapIndex==0?1:0;model.changed();return true;}
    if(ctrl&&key.getModifiers().isShiftDown()&&code=='M'){createClip();return true;}
    if(ctrl&&(code=='Z'||code=='Y')){if(code=='Y'||key.getModifiers().isShiftDown())model.redo();else model.undo();return true;}
    if(ctrl&&code=='L'){if(selectionEnd>selectionStart&&onLoop)onLoop(selectionStart,selectionEnd);return true;}
    if(ctrl&&code=='A'){selectionStart=0;selectionEnd=model.barTicks;for(const auto& c:model.clips)selectionEnd=std::max(selectionEnd,c.start+c.length);for(const auto& c:model.audioClips)selectionEnd=std::max(selectionEnd,c.start+c.length);firstTrack=1;lastTrack=laneIds.back();repaint();return true;}
    if(ctrl&&code=='D'){selectionStart=selectionEnd=0;repaint();return true;}
    if(ctrl&&code=='E')
    {
        if(model.audioClips.size()>=audio::maximumVoices)return true;
        model.checkpoint();std::vector<MidiClip> result;
        for(const auto& c:model.clips)if(c.track==anchorTrack&&cursorTime>c.start&&cursorTime<c.start+c.length){result.push_back(model.slice(c,c.start,cursorTime));result.push_back(model.slice(c,cursorTime,c.start+c.length));}else result.push_back(c);
        model.clips=std::move(result);model.active=0;
        std::vector<AudioClip> audioResult;
        for(const auto& c:model.audioClips)if(c.track==anchorTrack&&cursorTime>c.start&&cursorTime<c.start+c.length){audioResult.push_back(model.sliceAudio(c,c.start,cursorTime,model.tempo));audioResult.push_back(model.sliceAudio(c,cursorTime,c.start+c.length,model.tempo));}else audioResult.push_back(c);
        model.audioClips=std::move(audioResult);model.activeAudio=0;model.changed();return true;
    }
    if(ctrl&&(code=='C'||code=='X'||code=='B'))
    {
        audioClipboard.clear();clipboard.clear();for(const auto& c:model.clips)
        {
            if(selectionEnd>selectionStart){if(c.track>=firstTrack&&c.track<=lastTrack&&c.start<selectionEnd&&c.start+c.length>selectionStart)clipboard.push_back(model.slice(c,std::max(c.start,selectionStart),std::min(c.start+c.length,selectionEnd)));}
            else if(c.id==model.active)clipboard.push_back(c);
        }
        for(const auto& c:model.audioClips)
        {
            if(selectionEnd>selectionStart){if(c.track>=firstTrack&&c.track<=lastTrack&&c.start<selectionEnd&&c.start+c.length>selectionStart)audioClipboard.push_back(model.sliceAudio(c,std::max(c.start,selectionStart),std::min(c.start+c.length,selectionEnd),model.tempo));}
            else if(c.id==model.activeAudio)audioClipboard.push_back(c);
        }
        if(code=='C')return true;
    }
    if(code==juce::KeyPress::deleteKey||(ctrl&&code=='X'))
    {
        model.checkpoint();if(selectionEnd>selectionStart)model.deleteRange(selectionStart,selectionEnd,firstTrack,lastTrack);else{std::erase_if(model.clips,[this](const auto& c){return c.id==model.active;});model.active=0;std::erase_if(model.audioClips,[this](const auto& c){return c.id==model.activeAudio;});model.activeAudio=0;}model.changed();return true;
    }
    if(ctrl&&(code=='V'||code=='B'))
    {
        if(clipboard.empty()&&audioClipboard.empty())return true;model.checkpoint();Tick start=editing::maximumTime,end=0;
        for(const auto& c:clipboard){start=std::min(start,c.start);end=std::max(end,c.start+c.length);}
        for(const auto& c:audioClipboard){start=std::min(start,c.start);end=std::max(end,c.start+c.length);}
        const Tick delta=(code=='B'?end:cursorTime)-start;
        for(auto c:clipboard)if(c.start+delta>=0&&c.start+delta+c.length<=editing::maximumTime&&model.clips.size()<editing::maximumClips){c.id=model.freshClipId();c.start+=delta;model.clips.push_back(c);model.active=c.id;}
        for(auto c:audioClipboard)if(c.start+delta>=0&&c.start+delta+c.length<=editing::maximumTime&&model.audioClips.size()<audio::maximumVoices){c.id=model.freshClipId();c.start+=delta;model.audioClips.push_back(c);model.activeAudio=c.id;}
        selectionStart=selectionEnd=0;model.changed();return true;
    }
    return false;
}
}




