#include "PianoRoll.h"
#include <cmath>
namespace auralis
{
namespace
{
bool blackKey(int pitch) { const int n = pitch%12; return n == 1 || n == 3 || n == 6 || n == 8 || n == 10; }
juce::String pitchName(int pitch)
{
    static const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    return juce::String(names[pitch%12])+juce::String(pitch/12-1);
}
}
PianoRoll::PianoRoll(MidiProject& state) : model(state)
{
    setWantsKeyboardFocus(true);
    tools.addItemList({"Draw  P", "Paint  B", "Select  E", "Delete  D", "Slice  C", "Mute  T"},1);
    tools.setSelectedId(1);
    for (int i=0; i<12; ++i) snap.addItem(editing::snapNames[i],i+1);
    snap.setSelectedId(model.snapIndex+1);
    snap.onChange = [this] { model.snapIndex = snap.getSelectedId()-1; model.changed(); };
    property.addItemList({"Velocity", "Note panning", "Note fine pitch", "Channel volume", "Channel panning", "Channel pitch"},1);
    property.setSelectedId(1); property.onChange = [this] { repaint(); };
    cycle.addItemList({"Loop: 1 bar", "Loop: 2 bars", "Loop: 4 bars", "Loop: 8 bars"},1);
    cycle.setSelectedId(1);
    cycle.onChange = [this]
    {
        if (auto* c = model.clip(model.active))
        {
            const Tick length = model.barTicks * (1LL << (cycle.getSelectedId()-1));
            // Reject destructive loop shortening; notes/events remain intact.
            Tick end = 0; for (const auto& n : c->notes) end = std::max(end,n.start+n.length);
            for (const auto& event : c->events) end = std::max(end,event.time+editing::minimumNote);
            if (length < end) { refresh(); return; }
            model.checkpoint(); c->cycle = length; c->offset %= length; model.changed();
        }
    };
    configureButton(quantize,"Quantize", "Ctrl+Q: snap selected notes (or all notes if none selected) to the current grid. Undo: Ctrl+Z.");
    quantize.onClick = [this] { quantizeNotes(); };
    setHelp(tools,"Piano-roll tools", "P Draw, B Paint, E Select, D Delete, C Slice, T Mute. Ctrl+right-drag selects; right-drag erases. Alt bypasses snap.");
    setHelp(snap,"Shared snap grid", "Choose grid spacing shared by piano roll and arrangement. Backspace toggles Line/None; hold Alt to bypass. Step is a sixteenth note; Beat is a quarter note; Bar follows signature.");
    setHelp(property,"Control lane / F or Shift+F", "F cycles properties. Velocity, note pan and fine pitch belong to notes; channel volume/pan/pitch are clip-local event curves. Alt+wheel edits the chosen note property. No MIDI or audio output yet.");
    setHelp(cycle,"Clip loop length", "Set the independently owned source loop length. Extending the arrangement clip repeats this loop. Shortening across existing notes/events is rejected to preserve data.");
    setHelp(horizontal,"Piano-roll horizontal scroll", "Scroll in time. Shift+wheel scrolls horizontally; Ctrl+wheel or Page Up/Down zooms time; middle-drag pans both axes.");
    setHelp(vertical,"Piano-roll pitch scroll", "Wheel scrolls pitch; Ctrl+Alt+wheel changes note row height.");
    for (auto* c : std::initializer_list<juce::Component*>{&tools,&snap,&property,&cycle,&quantize,&horizontal,&vertical,&controlDivider}) addAndMakeVisible(c);
    horizontal.addListener(this); vertical.addListener(this);
    horizontal.setAutoHide(false); vertical.setAutoHide(false);
    setHelp(controlDivider,"Control lane divider","Drag to resize the property editor. Double-click resets its height.");
    controlDivider.onDrag = [this](int delta) { controlHeight -= delta; resized(); };
    controlDivider.onReset = [this] { controlHeight = editing::defaultControlHeight; resized(); };
}
void PianoRoll::refresh()
{
    if (activeSeen != model.active) { activeSeen = model.active; selected.clear(); offset = 0; }
    snap.setSelectedId(model.snapIndex+1,juce::dontSendNotification);
    if (const auto* c = model.clip(model.active))
    {
        for (auto it = selected.begin(); it != selected.end();)
            if (std::none_of(c->notes.begin(),c->notes.end(),[&](const auto& n){return n.id == *it;})) it = selected.erase(it); else ++it;
        int id = 1; while (id < 4 && model.barTicks*(1LL<<(id-1)) < c->cycle) ++id;
        cycle.setSelectedId(id,juce::dontSendNotification);
    }
    updateRanges(); repaint();
}
void PianoRoll::resized()
{
    tools.setBounds(8,5,102,26); snap.setBounds(116,5,100,26);
    cycle.setBounds(222,5,116,26); quantize.setBounds(344,5,83,26);
    controlHeight = juce::jlimit(editing::minControlHeight,juce::jmax(editing::minControlHeight,getHeight()/2),controlHeight);
    grid = {editing::keyboardWidth,editing::toolbarHeight+editing::rulerHeight,getWidth()-editing::keyboardWidth-12,juce::jmax(30,getHeight()-editing::toolbarHeight-editing::rulerHeight-controlHeight-20)};
    controlDivider.setBounds(0,grid.getBottom(),getWidth(),6);
    controls = {editing::keyboardWidth,grid.getBottom()+34,grid.getWidth(),juce::jmax(20,getHeight()-grid.getBottom()-50)};
    property.setBounds(8,grid.getBottom()+7,184,24);
    horizontal.setBounds(editing::keyboardWidth,getHeight()-11,grid.getWidth(),10);
    vertical.setBounds(getWidth()-11,grid.getY(),10,grid.getHeight()); updateRanges();
}
void PianoRoll::updateRanges()
{
    const auto* c = model.clip(model.active);
    horizontal.setRangeLimits(0,c ? std::max(16.0,static_cast<double>(c->cycle)/editing::ppq+4) : 16);
    horizontal.setCurrentRange(offset,std::max(1.0,grid.getWidth()/pixels),juce::dontSendNotification); offset = horizontal.getCurrentRangeStart();
    vertical.setRangeLimits(0,128); vertical.setCurrentRange(pitchOffset,std::max(1.0,static_cast<double>(grid.getHeight())/rowHeight),juce::dontSendNotification); pitchOffset = vertical.getCurrentRangeStart();
}
void PianoRoll::scrollBarMoved(juce::ScrollBar* bar,double start)
{
    if (bar == &horizontal) offset = start; else pitchOffset = start; repaint();
}
Tick PianoRoll::timeAt(int x,bool bypass) const { return model.snap(static_cast<Tick>((offset+(x-grid.getX())/pixels)*editing::ppq),bypass); }
int PianoRoll::pitchAt(int y) const { return juce::jlimit(0,127,127-static_cast<int>(pitchOffset+(y-grid.getY())/static_cast<double>(rowHeight))); }
juce::Rectangle<int> PianoRoll::noteBounds(const MidiNote& n) const
{
    return {grid.getX()+juce::roundToInt((static_cast<double>(n.start)/editing::ppq-offset)*pixels),grid.getY()+juce::roundToInt((127-n.pitch-pitchOffset)*rowHeight),juce::jmax(3,juce::roundToInt(static_cast<double>(n.length)/editing::ppq*pixels)),rowHeight-1};
}
MidiNote* PianoRoll::noteAt(juce::Point<int> p)
{
    if (auto* c = model.clip(model.active)) for (auto it=c->notes.rbegin(); it!=c->notes.rend(); ++it) if (noteBounds(*it).contains(p)) return &*it;
    return nullptr;
}
HelpContent PianoRoll::helpAt(juce::Point<int> p,bool) const
{
    if (controls.contains(p)) return {"Control editor", "Drag velocity/pan/fine-pitch stems at note starts. Channel controls draw clip-local event points; right-drag removes points. F cycles property. Changes affect only this clip. Ctrl+Z undoes the gesture."};
    return {"Piano roll / independent clip", "Click to add notes; drag notes to move, right edge to resize. Ctrl+right-drag selects. Right-drag erases. Delete removes selection. Ctrl+A/C/X/V/B: select/copy/cut/paste/duplicate. Ctrl+Z undo; Ctrl+Y redo. Wheel scrolls, Shift+wheel scrolls time, Ctrl+wheel zooms. F1 opens all shortcuts. No sound yet."};
}
void PianoRoll::paint(juce::Graphics& g)
{
    panel(g,getLocalBounds());
    const auto* c = model.clip(model.active);
    text(g,c ? juce::String(c->name)+"  /  Track "+juce::String(c->track) : "Select or create a MIDI clip",{440,5,getWidth()-450,26},11,design::colour::mint);
    g.setColour(colour(design::colour::background)); g.fillRect(grid); g.fillRect(controls);
    for (int p=0;p<128;++p)
    {
        const int y=grid.getY()+juce::roundToInt((127-p-pitchOffset)*rowHeight);
        if (y+rowHeight < grid.getY() || y>=grid.getBottom()) continue;
        const juce::Rectangle<int> row{grid.getX(),y,grid.getWidth(),rowHeight};
        g.setColour(colour(blackKey(p)?design::colour::background:design::colour::panel)); g.fillRect(row.getIntersection(grid));
        g.setColour(colour(design::colour::line).withAlpha(0.4f)); g.drawHorizontalLine(y,static_cast<float>(grid.getX()),static_cast<float>(grid.getRight()));
        g.setColour(blackKey(p)?juce::Colour(0xff303842):juce::Colour(0xffb9c3cc));
        auto key=juce::Rectangle<int>(1,y,editing::keyboardWidth-3,rowHeight-1).getIntersection({0,grid.getY(),editing::keyboardWidth,grid.getHeight()});
        g.fillRect(key);
        if (rowHeight>=12 && p%12==0) text(g,pitchName(p),key,10,0xff17212b,true,juce::Justification::centredRight);
    }
    Tick division = model.step(); if (division == 0) division = editing::ppq/4;
    while (static_cast<double>(division)/editing::ppq*pixels < 8) division*=2;
    const Tick first = static_cast<Tick>(offset*editing::ppq)/division*division;
    for (Tick t=first;t<(offset+grid.getWidth()/pixels)*editing::ppq;t+=division)
    {
        const int x=grid.getX()+juce::roundToInt((static_cast<double>(t)/editing::ppq-offset)*pixels);
        const bool bar=t%model.barTicks==0;
        g.setColour(colour(design::colour::line).withAlpha(bar?1.0f:0.35f)); g.drawVerticalLine(x,static_cast<float>(grid.getY()),static_cast<float>(controls.getBottom()));
        if(bar) text(g,juce::String(static_cast<int>(t/model.barTicks)+1),{x+4,editing::toolbarHeight,45,editing::rulerHeight},10,design::colour::muted);
    }
    if (!c) { text(g,"Double-right-click an instrument lane to create a MIDI clip",grid.reduced(15),14,design::colour::muted,false,juce::Justification::centred); return; }
    const int endX=grid.getX()+juce::roundToInt((static_cast<double>(c->cycle)/editing::ppq-offset)*pixels);
    { juce::Graphics::ScopedSaveState save(g); g.reduceClipRegion(grid);
      if(endX<grid.getRight()){g.setColour(juce::Colours::black.withAlpha(0.3f));g.fillRect(endX,grid.getY(),grid.getRight()-endX,grid.getHeight());}
      for(const auto& n:c->notes)
      {
          auto r=noteBounds(n); g.setColour(colour(selected.count(n.id)?design::colour::amber:design::colour::mint).withAlpha(n.muted?0.3f:0.9f));g.fillRoundedRectangle(r.toFloat().reduced(1),2);
          if(r.getWidth()>30 && rowHeight>=14) text(g,pitchName(n.pitch),r.reduced(4,0),10,design::colour::background,true);
      }
      if(drag==Drag::selection){g.setColour(colour(design::colour::mint).withAlpha(0.15f));g.fillRect(juce::Rectangle<int>(anchor,cursor));g.setColour(colour(design::colour::mint));g.drawRect(juce::Rectangle<int>(anchor,cursor));}
    }
    { juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(controls);
      const int target=property.getSelectedId();
      if(target<=3) for(const auto& n:c->notes)
      {
          const int x=noteBounds(n).getX();
          const double v=target==1?n.velocity/127.0:target==2?(n.pan+100)/200.0:(n.finePitch+1200)/2400.0;
          const int y=controls.getBottom()-juce::roundToInt(v*(controls.getHeight()-8));
          g.setColour(colour(selected.count(n.id)?design::colour::amber:design::colour::mint));g.drawLine(static_cast<float>(x),static_cast<float>(controls.getBottom()),static_cast<float>(x),static_cast<float>(y),2);g.fillEllipse(static_cast<float>(x-3),static_cast<float>(y-3),6,6);
      }
      else
      {
          auto events=c->events;std::sort(events.begin(),events.end(),[](const auto& a,const auto& b){return a.time<b.time;});
          int previousX=controls.getX(), previousY=controls.getBottom()-controls.getHeight()/2;
          g.setColour(colour(design::colour::amber));
          for(const auto& event:events) if(static_cast<int>(event.target)==target-4)
          {
              const double normalized=target==4?event.value:(event.value+1)/2;
              const int x=grid.getX()+juce::roundToInt((static_cast<double>(event.time)/editing::ppq-offset)*pixels), y=controls.getBottom()-juce::roundToInt(normalized*(controls.getHeight()-8));
              g.drawLine(static_cast<float>(previousX),static_cast<float>(previousY),static_cast<float>(x),static_cast<float>(previousY),1);
              g.drawLine(static_cast<float>(x),static_cast<float>(previousY),static_cast<float>(x),static_cast<float>(y),1);g.fillEllipse(static_cast<float>(x-3),static_cast<float>(y-3),6,6);previousX=x;previousY=y;
          }
          g.drawLine(static_cast<float>(previousX),static_cast<float>(previousY),static_cast<float>(controls.getRight()),static_cast<float>(previousY),1);
      }
    }
}
void PianoRoll::eraseAt(juce::Point<int> p)
{
    if(auto* n=noteAt(p)){const int id=n->id;auto* c=model.clip(model.active);std::erase_if(c->notes,[id](const auto& note){return note.id==id;});selected.erase(id);}
}
void PianoRoll::drawAt(const juce::MouseEvent& e)
{
    if(!grid.contains(e.getPosition())) return;
    if(auto* c=model.clip(model.active))
    {
        const Tick t=timeAt(e.x,e.mods.isAltDown());const int pitch=pitchAt(e.y);
        if(t>=c->cycle) return;
        if(std::any_of(c->notes.begin(),c->notes.end(),[&](const auto& n){return n.start==t&&n.pitch==pitch;})) return;
        model.addNote(*c,t,pitch,lastLength);
    }
}
void PianoRoll::controlAt(juce::Point<int> p)
{
    auto* c=model.clip(model.active);if(!c)return;
    const double value=juce::jlimit(0.0,1.0,static_cast<double>(controls.getBottom()-p.y)/juce::jmax(1,controls.getHeight()-8));
    const int target=property.getSelectedId();
    if(target<=3) for(auto& n:c->notes)
    {
        if(std::abs(noteBounds(n).getX()-p.x)>7 || (!selected.empty()&&!selected.count(n.id)))continue;
        if(target==1)n.velocity=juce::jlimit(1,127,juce::roundToInt(value*127));
        if(target==2)n.pan=juce::roundToInt(value*200-100);
        if(target==3)n.finePitch=juce::roundToInt(value*2400-1200);
    }
    else
    {
        const Tick time=timeAt(p.x);if(time>=c->cycle)return;
        const auto type=static_cast<EventTarget>(target-4);
        std::erase_if(c->events,[&](const auto& event){return event.target==type&&event.time==time;});
        if(!juce::ModifierKeys::getCurrentModifiersRealtime().isRightButtonDown() && c->events.size()<editing::maximumNotes)
            c->events.push_back({time,type,target==4?value:value*2-1});
    }
}
void PianoRoll::mouseDown(const juce::MouseEvent& e)
{
    grabKeyboardFocus();anchor=cursor=e.getPosition();anchorTime=timeAt(e.x,e.mods.isAltDown());anchorPitch=pitchAt(e.y);
    if(e.mods.isMiddleButtonDown()){drag=Drag::pan;initialOffset=offset;initialPitchOffset=pitchOffset;return;}
    auto* c=model.clip(model.active);if(!c)return;
    if(controls.contains(anchor)){model.checkpoint();drag=Drag::control;controlAt(anchor);model.changed();return;}
    if(!grid.contains(anchor))return;
    if(e.mods.isCtrlDown()||tools.getSelectedId()==3)
    {
        previousSelection=e.mods.isShiftDown()?selected:std::set<int>{};selected=previousSelection;drag=Drag::selection;
        if(auto* n=noteAt(anchor))selected.insert(n->id);repaint();return;
    }
    model.checkpoint();
    if(e.mods.isRightButtonDown()||tools.getSelectedId()==4){drag=Drag::erase;eraseAt(anchor);model.changed();return;}
    if(auto* n=noteAt(anchor))
    {
        if(tools.getSelectedId()==6){n->muted=!n->muted;model.changed();return;}
        if(tools.getSelectedId()==5)
        {
            const Tick cut=anchorTime;if(cut>n->start&&cut<n->start+n->length)
            {auto tail=*n;tail.id=model.freshNoteId();tail.length=n->start+n->length-cut;tail.start=cut;n->length=cut-n->start;c->notes.push_back(tail);}model.changed();return;
        }
        lastLength=n->length;dragId=n->id;
        if(!selected.count(n->id)){selected.clear();selected.insert(n->id);}
        drag=e.x>=noteBounds(*n).getRight()-6?Drag::resize:Drag::move;originals=c->notes;
    }
    else if(tools.getSelectedId()==1||tools.getSelectedId()==2)
    {
        drawAt(e);drag=tools.getSelectedId()==2?Drag::paint:Drag::none;
        if(!c->notes.empty()){selected={c->notes.back().id};dragId=c->notes.back().id;originals=c->notes;if(drag==Drag::none)drag=e.mods.isShiftDown()?Drag::resize:Drag::move;}
    }
    model.changed();
}
void PianoRoll::mouseDrag(const juce::MouseEvent& e)
{
    cursor=e.getPosition();
    if(drag==Drag::pan){offset=std::max(0.0,initialOffset-(e.x-anchor.x)/pixels);pitchOffset=initialPitchOffset-(e.y-anchor.y)/static_cast<double>(rowHeight);updateRanges();repaint();return;}
    auto* c=model.clip(model.active);if(!c)return;
    if(drag==Drag::selection)
    {
        selected=previousSelection;const juce::Rectangle<int> box(anchor,cursor);
        for(const auto& n:c->notes)if(box.intersects(noteBounds(n)))selected.insert(n.id);repaint();return;
    }
    if(drag==Drag::erase)
    {
        // Sample the swept segment so fast drags do not skip narrow notes.
        const auto previous=anchor;const int steps=juce::jmax(1,previous.getDistanceFrom(cursor));
        for(int i=0;i<=steps;++i)eraseAt(previous+(cursor-previous)*i/steps);anchor=cursor;
    }
    if(drag==Drag::paint)drawAt(e);
    if(drag==Drag::control)controlAt(cursor);
    if(drag==Drag::move||drag==Drag::resize)
    {
        Tick delta=timeAt(e.x,e.mods.isAltDown())-anchorTime;int pitchDelta=pitchAt(e.y)-anchorPitch;
        for(const auto& n:originals)if(selected.count(n.id))
        {
            if(drag==Drag::move){delta=std::clamp<Tick>(delta,-n.start,c->cycle-n.start-n.length);pitchDelta=std::clamp(pitchDelta,-n.pitch,127-n.pitch);}
            else delta=std::clamp<Tick>(delta,editing::minimumNote-n.length,c->cycle-n.start-n.length);
        }
        for(auto& n:c->notes)if(selected.count(n.id))for(const auto& old:originals)if(old.id==n.id)
        {if(drag==Drag::move){n.start=old.start+delta;n.pitch=old.pitch+pitchDelta;}else{n.length=old.length+delta;lastLength=n.length;}break;}
    }
    model.changed();
}
void PianoRoll::mouseUp(const juce::MouseEvent&) { drag=Drag::none;repaint(); }
void PianoRoll::zoom(double factor){pixels=juce::jlimit(editing::minPixelsPerBeat,editing::maxPixelsPerBeat,pixels*factor);updateRanges();repaint();}
void PianoRoll::mouseWheelMove(const juce::MouseEvent& e,const juce::MouseWheelDetails& w)
{
    if(e.mods.isCtrlDown()&&e.mods.isAltDown()){rowHeight=juce::jlimit(editing::minRowHeight,editing::maxRowHeight,rowHeight+(w.deltaY>0?1:-1));updateRanges();repaint();}
    else if(e.mods.isCtrlDown())zoom(w.deltaY>0?1.2:1/1.2);
    else if(e.mods.isAltDown())
    {
        if(auto* c=model.clip(model.active))
        {model.checkpoint();auto* hovered=noteAt(e.getPosition());for(auto& n:c->notes)if(selected.count(n.id)||(selected.empty()&&hovered&&n.id==hovered->id))
        {const int delta=w.deltaY>0?1:-1;if(property.getSelectedId()==1)n.velocity=juce::jlimit(1,127,n.velocity+delta);if(property.getSelectedId()==2)n.pan=juce::jlimit(-100,100,n.pan+delta);if(property.getSelectedId()==3)n.finePitch=juce::jlimit(-1200,1200,n.finePitch+delta*10);}model.changed();}
    }
    else if(e.mods.isShiftDown()||std::abs(w.deltaX)>std::abs(w.deltaY))horizontal.setCurrentRangeStart(offset-(e.mods.isShiftDown()?w.deltaY:w.deltaX)*8);
    else vertical.setCurrentRangeStart(pitchOffset-w.deltaY*18);
}
void PianoRoll::quantizeNotes()
{
    if(auto* c=model.clip(model.active)){model.checkpoint();for(auto& n:c->notes)if(chosen(n))n.start=std::min(model.snap(n.start),c->cycle-n.length);model.changed();}
}
bool PianoRoll::keyPressed(const juce::KeyPress& key)
{
    juce::File::getSpecialLocation(juce::File::currentExecutableFile).getSiblingFile("key-debug.txt").appendText("PianoRoll " + juce::String(key.getKeyCode())+" mods="+juce::String(key.getModifiers().getRawFlags())+" text="+juce::String(static_cast<int>(key.getTextCharacter()))+"\n"); // TEMP_KEY_DIAGNOSTIC
    const int rawCode=key.getKeyCode();const int code=(rawCode>='a'&&rawCode<='z')?rawCode-'a'+'A':rawCode;const bool ctrl=key.getModifiers().isCtrlDown();
    if(code==juce::KeyPress::pageUpKey||code==juce::KeyPress::pageDownKey){zoom(code==juce::KeyPress::pageUpKey?1.25:0.8);return true;}
    if(code==juce::KeyPress::backspaceKey){model.snapIndex=model.snapIndex==0?1:0;model.changed();return true;}
    if(!ctrl&&code=='F'){property.setSelectedId(property.getSelectedId()%6+1);return true;}
    if(!ctrl){const juce::String keys="PBEDCT";const int index=keys.indexOfChar(static_cast<juce::juce_wchar>(code));if(index>=0){tools.setSelectedId(index+1);return true;}}
    auto* c=model.clip(model.active);if(!c)return false;
    if(ctrl&&code=='A'){selected.clear();for(const auto& n:c->notes)selected.insert(n.id);repaint();return true;}
    if(ctrl&&code=='D'){selected.clear();repaint();return true;}
    if(ctrl&&code=='Q'){quantizeNotes();return true;}
    if(ctrl&&(code=='C'||code=='X'))
    {clipboard.clear();for(const auto& n:c->notes)if(selected.count(n.id))clipboard.push_back(n);if(code=='C')return true;}
    if(code==juce::KeyPress::deleteKey||(ctrl&&code=='X'))
    {model.checkpoint();std::erase_if(c->notes,[this](const auto& n){return selected.count(n.id)!=0;});selected.clear();model.changed();return true;}
    if(ctrl&&(code=='V'||code=='B'))
    {
        auto copy=clipboard;if(code=='B'){copy.clear();for(const auto& n:c->notes)if(selected.count(n.id))copy.push_back(n);}
        if(copy.empty())return true;
        Tick first=copy.front().start,end=0;for(const auto& n:copy){first=std::min(first,n.start);end=std::max(end,n.start+n.length);}
        const Tick shift=code=='B'?end-first:std::max<Tick>(0,anchorTime)-first;
        model.checkpoint();selected.clear();
        const Tick needed=end+shift;
        if(needed>c->cycle&&needed<=editing::maximumTime){c->cycle=((needed+model.barTicks-1)/model.barTicks)*model.barTicks;c->length=std::max(c->length,c->cycle);}
        for(auto n:copy)if(n.start+shift>=0&&n.start+shift+n.length<=c->cycle&&c->notes.size()<editing::maximumNotes){n.start+=shift;n.id=model.freshNoteId();selected.insert(n.id);c->notes.push_back(n);}model.changed();return true;
    }
    if(code==juce::KeyPress::leftKey||code==juce::KeyPress::rightKey||code==juce::KeyPress::upKey||code==juce::KeyPress::downKey)
    {
        model.checkpoint();Tick delta=code==juce::KeyPress::leftKey?-std::max(editing::minimumNote,model.step()):code==juce::KeyPress::rightKey?std::max(editing::minimumNote,model.step()):0;
        int transpose=code==juce::KeyPress::upKey?(ctrl?12:1):code==juce::KeyPress::downKey?(ctrl?-12:-1):0;
        for(const auto& n:c->notes)if(selected.count(n.id)){delta=std::clamp<Tick>(delta,-n.start,c->cycle-n.start-n.length);transpose=std::clamp(transpose,-n.pitch,127-n.pitch);}
        for(auto& n:c->notes)if(selected.count(n.id)){n.start+=delta;n.pitch+=transpose;}model.changed();return true;
    }
    return false;
}
}
