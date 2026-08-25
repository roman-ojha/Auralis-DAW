#include "PianoRoll.h"
#include "model/MidiEditing.h"
#include "EditorKeys.h"
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
    tools.addItemList({"Draw  P", "Paint  B", "Select  E", "Delete  D", "Slice  C", "Mute  T", "Zoom  Z"},1);
    tools.setSelectedId(1,juce::dontSendNotification);
    const ControlIcon icons[] = {ControlIcon::draw,ControlIcon::paint,ControlIcon::select,ControlIcon::erase,ControlIcon::slice,ControlIcon::noteMute,ControlIcon::zoom};
    const char* names[] = {"Draw / P", "Paint / B", "Select / E", "Erase / D", "Slice / C", "Mute / T", "Zoom / Z"};
    const char* help[] = {"P: draw notes. Drag to move, edges to resize. Shift-drag an existing note clones the selection.",
        "B: paint a sequence by dragging. Right-drag erases. Ctrl+right-drag selects.",
        "E: drag a rectangle to select notes. Ctrl+right-drag selects with any tool. Shift adds to selection.",
        "D: sweep to erase notes. Delete removes selected notes. Ctrl+Z undoes the gesture.",
        "C: drag a cut line across notes, then release to split them. Ctrl+Z undoes the cut.",
        "T: click or sweep notes to toggle mute once per gesture. Muted notes are dimmed.",
        "Z: drag a rectangle to zoom into its time and pitch range. Shift+5 fits selected notes; Shift+4 fits all."};
    for (int i=0;i<7;++i)
    {
        auto button=std::make_unique<IconButton>(icons[i],names[i],help[i]);
        button->onClick=[this,i]{setTool(i+1);grabKeyboardFocus();};
        addAndMakeVisible(*button);toolButtons.push_back(std::move(button));
    }
    tools.onChange=[this]{setTool(tools.getSelectedId());};
    setTool(1);
    configureButton(newClip,"+ Clip","Ctrl+Shift+M: create an independent MIDI clip on the selected instrument track. Clicking an empty piano-roll grid also creates a clip.");
    newClip.onClick=[this]{if(onCreateClip)onCreateClip();grabKeyboardFocus();};
    configureButton(actions,"Tools...","Editing commands: legato, glue, chop, reverse, flip, strum, arpeggiate, scales and chords. Each edit supports Ctrl+Z.");
    actions.onClick=[this]{showActions();};
    configureButton(fit,"Fit","Shift+5: fit selected notes, or all notes when none are selected. Shift+4 fits all notes.");
    fit.onClick=[this]{fitNotes();grabKeyboardFocus();};
    for (int i=0; i<12; ++i) snap.addItem(editing::snapNames[i],i+1);
    snap.setSelectedId(model.snapIndex+1,juce::dontSendNotification);
    snap.onChange = [this] { model.snapIndex = snap.getSelectedId()-1; model.changed(); };
    property.addItemList({"Velocity", "Note panning", "Note fine pitch", "Channel volume", "Channel panning", "Channel pitch"},1);
    property.setSelectedId(1,juce::dontSendNotification); property.onChange = [this] { repaint(); };
    cycle.addItemList({"Loop: 1 bar", "Loop: 2 bars", "Loop: 4 bars", "Loop: 8 bars"},1);
    cycle.setSelectedId(1,juce::dontSendNotification);
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
    setHelp(property,"Control lane / F or Shift+F", "F cycles properties. Velocity, note pan and fine pitch belong to notes; channel volume/pan/pitch are clip-local event curves. Alt+wheel edits the chosen note property. Loaded instruments play notes through the mixer; complete channel-controller DSP mapping remains unfinished.");
    setHelp(cycle,"Clip loop length", "Set the independently owned source loop length. Extending the arrangement clip repeats this loop. Shortening across existing notes/events is rejected to preserve data.");
    setHelp(horizontal,"Piano-roll horizontal scroll", "Scroll in time. Shift+wheel scrolls horizontally; Ctrl+wheel or Page Up/Down zooms time; middle-drag pans both axes.");
    setHelp(vertical,"Piano-roll pitch scroll", "Wheel scrolls pitch; Ctrl+Alt+wheel changes note row height.");
    for (auto* c : std::initializer_list<juce::Component*>{&snap,&property,&cycle,&quantize,&horizontal,&vertical,&controlDivider,&newClip,&actions,&fit}) addAndMakeVisible(c);
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
    int x=8;
    for(auto& button:toolButtons){button->setBounds(x,4,editing::toolButtonWidth,28);x+=editing::toolButtonWidth+editing::toolButtonGap;}
    actions.setBounds(x+4,4,72,28);newClip.setBounds(x+80,4,70,28);fit.setBounds(x+154,4,44,28);
    snap.setBounds(8,39,110,26);cycle.setBounds(124,39,116,26);quantize.setBounds(246,39,84,26);
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
Tick PianoRoll::timeAt(int x,bool bypass) const { return model.snap(static_cast<Tick>((offset+(x-grid.getX())/pixels)*editing::ppq),bypass,pixels); }
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
    if(p.x<grid.getX()&&p.y>=grid.getY()&&p.y<grid.getBottom())return {"Piano keyboard", "Pitches align with note rows. Click a key to highlight its pitch; Ctrl-click selects notes of that pitch. Keys audition the loaded instrument through the track and Master. Wheel scrolls; Ctrl+Alt+wheel changes key height."};
    if (controls.contains(p)) return {"Control editor", "Drag velocity/pan/fine-pitch stems at note starts. Channel controls draw clip-local event points; right-drag removes points. F cycles property. Changes affect only this clip. Ctrl+Z undoes the gesture."};
    return {"Piano roll / independent clip", "Click to add notes; drag notes to move, either edge to resize. Ctrl+right-drag selects. Right-drag erases. Delete removes selection. Ctrl+A/C/X/V/B: select/copy/cut/paste/duplicate. Ctrl+Z undo; Ctrl+Y redo. Wheel scrolls, Shift+wheel scrolls time, Ctrl+wheel zooms. F1 opens all shortcuts. Load an instrument on the clip track for playback. F6 toggles computer keyboard audition."};
}
void PianoRoll::paint(juce::Graphics& g)
{
    panel(g,getLocalBounds());
    const auto* c = model.clip(model.active);
    text(g,c ? juce::String(c->name)+" / Track "+juce::String(c->track) : "Click the grid to create a clip",{340,39,getWidth()-350,26},11,design::colour::mint);
    g.setColour(colour(design::colour::background)); g.fillRect(grid); g.fillRect(controls);
    for (int p=0;p<128;++p)
    {
        const int y=grid.getY()+juce::roundToInt((127-p-pitchOffset)*rowHeight);
        if (y+rowHeight < grid.getY() || y>=grid.getBottom()) continue;
        const juce::Rectangle<int> row{grid.getX(),y,grid.getWidth(),rowHeight};
        g.setColour(colour(blackKey(p)?editing::darkRow:editing::lightRow)); g.fillRect(row.getIntersection(grid));
        if(p==hoverPitch){g.setColour(colour(design::colour::mint).withAlpha(0.08f));g.fillRect(row.getIntersection(grid));}
        g.setColour(colour(editing::gridLine)); g.drawHorizontalLine(y,static_cast<float>(grid.getX()),static_cast<float>(grid.getRight()));
    }
    // White keys extend under the shorter black keys, like a physical keyboard.
    {
        juce::Graphics::ScopedSaveState save(g);
        g.reduceClipRegion(0,grid.getY(),editing::keyboardWidth,grid.getHeight());
        for(int p=0;p<128;++p)if(!blackKey(p))
        {
            const int y=grid.getY()+juce::roundToInt((127-p-pitchOffset)*rowHeight);
            const int above=p<127&&blackKey(p+1)?rowHeight/2:0;
            const int below=p>0&&blackKey(p-1)?rowHeight/2:0;
            juce::Rectangle<int> key{0,y-above,editing::keyboardWidth-1,rowHeight+above+below};
            g.setColour(colour(p==hoverPitch?design::colour::mint:editing::whiteKey));g.fillRect(key);
            g.setColour(colour(editing::blackKey));g.drawHorizontalLine(key.getBottom()-1,0.0f,static_cast<float>(key.getRight()));
            if(rowHeight>=12)text(g,pitchName(p),{editing::keyboardBlackWidth+1,y,editing::keyboardWidth-editing::keyboardBlackWidth-4,rowHeight},9,editing::blackKey,p%12==0,juce::Justification::centredRight);
        }
        for(int p=0;p<128;++p)if(blackKey(p))
        {
            const int y=grid.getY()+juce::roundToInt((127-p-pitchOffset)*rowHeight);
            g.setColour(colour(p==hoverPitch?design::colour::mint:editing::blackKey));g.fillRect(0,y,editing::keyboardBlackWidth,rowHeight-1);
            g.setColour(juce::Colours::white.withAlpha(0.08f));g.drawHorizontalLine(y,0.0f,static_cast<float>(editing::keyboardBlackWidth));
        }
    }
    Tick division = model.gridStep(pixels); if (division == 0) division = editing::ppq/4;
    while (static_cast<double>(division)/editing::ppq*pixels < 8) division*=2;
    const Tick first = static_cast<Tick>(offset*editing::ppq)/division*division;
    for (Tick t=first;t<(offset+grid.getWidth()/pixels)*editing::ppq;t+=division)
    {
        const int x=grid.getX()+juce::roundToInt((static_cast<double>(t)/editing::ppq-offset)*pixels);
        const bool bar=t%model.barTicks==0;
        g.setColour(colour(editing::gridLine).withAlpha(bar?1.0f:0.55f));
        g.drawVerticalLine(x,static_cast<float>(grid.getY()),static_cast<float>(grid.getBottom()));
        g.drawVerticalLine(x,static_cast<float>(controls.getY()),static_cast<float>(controls.getBottom()));
        if(bar) text(g,juce::String(static_cast<int>(t/model.barTicks)+1),{x+4,editing::toolbarHeight,45,editing::rulerHeight},10,design::colour::muted);
    }
    text(g,"CONTROL",{5,controls.getY(),editing::keyboardWidth-8,22},9,design::colour::muted,true);
    if (!c) { text(g,"Click here to draw your first note",grid.reduced(15),14,design::colour::muted,false,juce::Justification::centred); return; }
    const int endX=grid.getX()+juce::roundToInt((static_cast<double>(c->cycle)/editing::ppq-offset)*pixels);
    { juce::Graphics::ScopedSaveState save(g); g.reduceClipRegion(grid);
      if(endX<grid.getRight()){g.setColour(juce::Colours::black.withAlpha(0.3f));g.fillRect(endX,grid.getY(),grid.getRight()-endX,grid.getHeight());}
      if(showGhosts)
      {
          g.setColour(colour(design::colour::muted).withAlpha(0.25f));
          for(const auto& other:model.clips)if(other.id!=c->id&&other.start<c->start-c->offset+c->cycle&&other.start+other.length>c->start-c->offset)
              for(auto note:MidiProject::renderedNotes(other))
              {
                  note.start+=other.start-c->start+c->offset;
                  if(note.start+note.length>0&&note.start<c->cycle)g.drawRoundedRectangle(noteBounds(note).toFloat().reduced(1),2,1);
              }
      }
      for(const auto& n:c->notes)
      {
          auto r=noteBounds(n); g.setColour(colour(selected.count(n.id)?design::colour::amber:design::colour::mint).withAlpha(n.muted?0.3f:0.9f));g.fillRoundedRectangle(r.toFloat().reduced(1),2);
          if(r.getWidth()>30 && rowHeight>=14) text(g,pitchName(n.pitch),r.reduced(4,0),10,design::colour::background,true);
      }
      if(drag==Drag::selection||drag==Drag::zoom){g.setColour(colour(design::colour::mint).withAlpha(0.15f));g.fillRect(juce::Rectangle<int>(anchor,cursor));g.setColour(colour(design::colour::mint));g.drawRect(juce::Rectangle<int>(anchor,cursor));}
      if(drag==Drag::slice){g.setColour(colour(design::colour::coral));g.drawLine(juce::Line<float>(anchor.toFloat(),cursor.toFloat()),2);}
      if(transportPlaying&&playhead>=c->start&&playhead<c->start+c->length)
      {
          const Tick local=(playhead-c->start+c->offset)%c->cycle;
          const int x=grid.getX()+juce::roundToInt((static_cast<double>(local)/editing::ppq-offset)*pixels);
          g.setColour(colour(design::colour::mint));g.drawVerticalLine(x,static_cast<float>(grid.getY()),static_cast<float>(grid.getBottom()));
      }
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
int PianoRoll::drawAt(const juce::MouseEvent& e)
{
    if(!grid.contains(e.getPosition())) return 0;
    if(auto* c=model.clip(model.active))
    {
        const Tick t=timeAt(e.x,e.mods.isAltDown());const int pitch=pitchAt(e.y);
        if(t>=c->cycle)
        {
            c->cycle=std::min(editing::maximumTime,((t+lastLength+model.barTicks-1)/model.barTicks)*model.barTicks);
            c->length=std::max(c->length,c->cycle);
        }
        if(t>=c->cycle) return 0;
        if(std::any_of(c->notes.begin(),c->notes.end(),[&](const auto& n){return n.start==t&&n.pitch==pitch;})) return 0;
        if(onAudition)onAudition(pitch);
        return model.addNote(*c,t,pitch,lastLength);
    }
    return 0;
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
        if(!controlErase && c->events.size()<editing::maximumNotes)
            c->events.push_back({time,type,target==4?value:value*2-1});
    }
}
void PianoRoll::mouseDown(const juce::MouseEvent& e)
{
    grabKeyboardFocus();drag=Drag::none;touched.clear();anchor=cursor=e.getPosition();anchorTime=timeAt(e.x,e.mods.isAltDown());anchorPitch=pitchAt(e.y);
    if(e.mods.isMiddleButtonDown()){drag=Drag::pan;initialOffset=offset;initialPitchOffset=pitchOffset;return;}
    if(!model.clip(model.active)&&grid.contains(anchor)&&e.mods.isLeftButtonDown()&&!e.mods.isCtrlDown()&&(tools.getSelectedId()==1||tools.getSelectedId()==2))
        if(onCreateClip)onCreateClip();
    auto* c=model.clip(model.active);if(!c)return;
    if(e.x<grid.getX()&&e.y>=grid.getY()&&e.y<grid.getBottom())
    {
        hoverPitch=anchorPitch;drag=Drag::keyboard;if(onAudition&&!e.mods.isCtrlDown())onAudition(hoverPitch);
        if(e.mods.isCtrlDown()){if(!e.mods.isShiftDown())selected.clear();for(const auto& n:c->notes)if(n.pitch==hoverPitch)selected.insert(n.id);}
        repaint();return;
    }
    if(controls.contains(anchor)){model.checkpoint();drag=Drag::control;controlErase=e.mods.isRightButtonDown();controlAt(anchor);model.changed();return;}
    if(!grid.contains(anchor))return;
    if(e.mods.isCtrlDown()||(tools.getSelectedId()==3&&!e.mods.isRightButtonDown()))
    {
        previousSelection=e.mods.isShiftDown()?selected:std::set<int>{};selected=previousSelection;drag=Drag::selection;
        if(auto* n=noteAt(anchor))selected.insert(n->id);repaint();return;
    }
    if(tools.getSelectedId()==7&&!e.mods.isRightButtonDown()){drag=Drag::zoom;return;}
    model.checkpoint();
    if(e.mods.isRightButtonDown()||tools.getSelectedId()==4){drag=Drag::erase;eraseAt(anchor);model.changed();return;}
    if(tools.getSelectedId()==5){drag=Drag::slice;repaint();return;}
    if(tools.getSelectedId()==6)
    {
        drag=Drag::mute;
        if(auto* n=noteAt(anchor)){n->muted=!n->muted;touched.insert(n->id);}
        model.changed();return;
    }
    if(auto* n=noteAt(anchor))
    {
        lastLength=n->length;dragId=n->id;
        if(!selected.count(n->id)){selected.clear();selected.insert(n->id);}
        const auto bounds=noteBounds(*n);
        drag=e.x>=bounds.getRight()-5?Drag::resize:e.x<bounds.getX()+4?Drag::resizeLeft:Drag::move;
        if(e.mods.isShiftDown()&&drag==Drag::move)
        {
            std::vector<MidiNote> clones;
            for(const auto& note:c->notes)if(selected.count(note.id)){auto copy=note;copy.id=model.freshNoteId();clones.push_back(copy);}
            if(c->notes.size()+clones.size()<=editing::maximumNotes)
            {
                selected.clear();for(const auto& copy:clones){selected.insert(copy.id);c->notes.push_back(copy);}
            }
        }
        originals=c->notes;
    }
    else if(tools.getSelectedId()==1||tools.getSelectedId()==2)
    {
        const int id=drawAt(e);drag=tools.getSelectedId()==2?Drag::paint:Drag::none;
        if(id){selected={id};dragId=id;originals=c->notes;if(drag==Drag::none)drag=e.mods.isShiftDown()?Drag::resize:Drag::move;}
    }
    model.changed();
}
void PianoRoll::mouseDrag(const juce::MouseEvent& e)
{
    const auto previous=cursor;cursor=e.getPosition();
    if(drag==Drag::pan){offset=std::max(0.0,initialOffset-(e.x-anchor.x)/pixels);pitchOffset=initialPitchOffset-(e.y-anchor.y)/static_cast<double>(rowHeight);updateRanges();repaint();return;}
    auto* c=model.clip(model.active);if(!c)return;
    if(drag==Drag::keyboard){hoverPitch=pitchAt(e.y);if(onAudition)onAudition(hoverPitch);repaint();return;}
    if(drag==Drag::slice||drag==Drag::zoom){repaint();return;}
    if(drag==Drag::selection)
    {
        selected=previousSelection;const juce::Rectangle<int> box(anchor,cursor);
        for(const auto& n:c->notes)if(box.intersects(noteBounds(n)))selected.insert(n.id);repaint();return;
    }
    if(drag==Drag::erase)
    {
        // Sample the swept segment so fast drags do not skip narrow notes.
        const int steps=juce::jmax(1,previous.getDistanceFrom(cursor));
        for(int i=0;i<=steps;++i)eraseAt(previous+(cursor-previous)*i/steps);
    }
    if(drag==Drag::paint||drag==Drag::control||drag==Drag::mute)
    {
        const int steps=juce::jmax(1,previous.getDistanceFrom(cursor));
        for(int i=0;i<=steps;++i)
        {
            const auto point=previous+(cursor-previous)*i/steps;
            if(drag==Drag::paint)drawAt(e.withNewPosition(point.toFloat()));
            else if(drag==Drag::control)controlAt(point);
            else if(auto* n=noteAt(point);n&&!touched.count(n->id)){n->muted=!n->muted;touched.insert(n->id);}
        }
    }
    if(drag==Drag::move||drag==Drag::resize||drag==Drag::resizeLeft)
    {
        Tick delta=timeAt(e.x,e.mods.isAltDown())-anchorTime;int pitchDelta=pitchAt(e.y)-anchorPitch;
        for(const auto& n:originals)if(selected.count(n.id))
        {
            if(drag==Drag::move){delta=std::clamp<Tick>(delta,-n.start,c->cycle-n.start-n.length);pitchDelta=std::clamp(pitchDelta,-n.pitch,127-n.pitch);}
            else if(drag==Drag::resizeLeft)delta=std::clamp<Tick>(delta,-n.start,n.length-editing::minimumNote);
            else delta=std::clamp<Tick>(delta,editing::minimumNote-n.length,c->cycle-n.start-n.length);
        }
        for(auto& n:c->notes)if(selected.count(n.id))for(const auto& old:originals)if(old.id==n.id)
        {if(drag==Drag::move){n.start=old.start+delta;n.pitch=old.pitch+pitchDelta;}else if(drag==Drag::resizeLeft){n.start=old.start+delta;n.length=old.length-delta;}else{n.length=old.length+delta;lastLength=n.length;}break;}
    }
    model.changed();
}
void PianoRoll::mouseUp(const juce::MouseEvent&)
{
    if(onAudition)onAudition(-1);
    if(drag==Drag::slice)sliceNotes();
    if(drag==Drag::zoom&&std::abs(cursor.x-anchor.x)>4)
    {
        const int high=pitchAt(std::min(anchor.y,cursor.y)),low=pitchAt(std::max(anchor.y,cursor.y));
        const double start=offset+(std::min(anchor.x,cursor.x)-grid.getX())/pixels;
        const double duration=std::abs(cursor.x-anchor.x)/pixels;
        pixels=juce::jlimit(editing::minPixelsPerBeat,editing::maxPixelsPerBeat,grid.getWidth()/duration);
        offset=std::max(0.0,start);
        if(std::abs(cursor.y-anchor.y)>4)
        {
            rowHeight=juce::jlimit(editing::minRowHeight,editing::maxRowHeight,grid.getHeight()/std::max(1,high-low+1));
            pitchOffset=127-high;
        }
        updateRanges();
    }
    drag=Drag::none;repaint();
}
void PianoRoll::mouseMove(const juce::MouseEvent& e)
{
    cursor=e.getPosition();
    hoverPitch=e.y>=grid.getY()&&e.y<grid.getBottom()?pitchAt(e.y):-1;
    auto pointer=juce::MouseCursor::NormalCursor;
    if(auto* note=noteAt(e.getPosition());note&&grid.contains(e.getPosition()))
    {
        const auto r=noteBounds(*note);
        pointer=e.x<r.getX()+4||e.x>=r.getRight()-5?juce::MouseCursor::LeftRightResizeCursor:juce::MouseCursor::DraggingHandCursor;
    }
    else if(grid.contains(e.getPosition()))pointer=juce::MouseCursor::CrosshairCursor;
    setMouseCursor(pointer);repaint();
}
void PianoRoll::mouseExit(const juce::MouseEvent&){hoverPitch=-1;repaint();}
void PianoRoll::mouseDoubleClick(const juce::MouseEvent& e)
{
    if(e.mods.isRightButtonDown()||e.mods.isCtrlDown())return;
    if(auto* note=noteAt(e.getPosition());note&&grid.contains(e.getPosition()))noteProperties(note->id);
}
void PianoRoll::noteProperties(int id)
{
    const auto* clip=model.clip(model.active);if(!clip)return;
    const auto found=std::find_if(clip->notes.begin(),clip->notes.end(),[id](const auto& n){return n.id==id;});
    if(found==clip->notes.end())return;
    auto* dialog=new juce::AlertWindow("Note properties","Edit this note. Values are clamped to the clip and MIDI ranges.",juce::MessageBoxIconType::NoIcon,this);
    dialog->addTextEditor("pitch",juce::String(found->pitch),"MIDI pitch (0-127)");
    dialog->addTextEditor("velocity",juce::String(found->velocity),"Velocity (1-127)");
    dialog->addTextEditor("pan",juce::String(found->pan),"Pan (-100 to 100)");
    dialog->addTextEditor("fine",juce::String(found->finePitch),"Fine pitch (cents)");
    for(const auto* field:{"pitch","velocity","pan","fine"})dialog->getTextEditor(field)->setInputRestrictions(5,"-0123456789");
    dialog->addButton("Apply",1,juce::KeyPress(juce::KeyPress::returnKey));dialog->addButton("Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey));
    const int clipId=clip->id;
    // JUCE owns the modal until its callback finishes; the editor and note are re-resolved.
    dialog->enterModalState(true,juce::ModalCallbackFunction::create([safe=juce::Component::SafePointer<PianoRoll>(this),window=juce::Component::SafePointer<juce::AlertWindow>(dialog),clipId,id](int result)
    {
        if(!safe||!window||result!=1)return;
        auto* current=safe->model.clip(clipId);if(!current)return;
        for(auto& note:current->notes)if(note.id==id)
        {
            safe->model.checkpoint();
            note.pitch=juce::jlimit(0,127,window->getTextEditorContents("pitch").getIntValue());
            note.velocity=juce::jlimit(1,127,window->getTextEditorContents("velocity").getIntValue());
            note.pan=juce::jlimit(-100,100,window->getTextEditorContents("pan").getIntValue());
            note.finePitch=juce::jlimit(-1200,1200,window->getTextEditorContents("fine").getIntValue());
            safe->model.changed();break;
        }
    }),true);
}
void PianoRoll::setPlayhead(Tick time,bool playing)
{
    if(playhead==time&&transportPlaying==playing)return;
    playhead=time;transportPlaying=playing;if(isVisible())repaint(grid);
}
void PianoRoll::zoom(double factor)
{
    const double pivot=grid.contains(cursor)?cursor.x-grid.getX():grid.getWidth()/2.0;
    const double time=offset+pivot/pixels;
    pixels=juce::jlimit(editing::minPixelsPerBeat,editing::maxPixelsPerBeat,pixels*factor);
    offset=std::max(0.0,time-pivot/pixels);updateRanges();repaint();
}
void PianoRoll::mouseWheelMove(const juce::MouseEvent& e,const juce::MouseWheelDetails& w)
{
    cursor=e.getPosition();
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
    if(auto* c=model.clip(model.active)){model.checkpoint();for(auto& n:c->notes)if(chosen(n))n.start=std::min(model.snap(n.start,false,pixels),c->cycle-n.length);model.changed();}
}
void PianoRoll::setTool(int id)
{
    tools.setSelectedId(id,juce::dontSendNotification);
    for(size_t i=0;i<toolButtons.size();++i)toolButtons[i]->setToggleState(static_cast<int>(i)+1==id,juce::dontSendNotification);
}
void PianoRoll::fitNotes()
{
    const auto* c=model.clip(model.active);if(!c)return;
    Tick first=c->cycle,last=0;int low=127,high=0;
    for(const auto& n:c->notes)if(chosen(n)){first=std::min(first,n.start);last=std::max(last,n.start+n.length);low=std::min(low,n.pitch);high=std::max(high,n.pitch);}
    if(last==0){first=0;last=c->cycle;low=48;high=84;}
    offset=std::max(0.0,static_cast<double>(first)/editing::ppq-0.25);
    pixels=juce::jlimit(editing::minPixelsPerBeat,editing::maxPixelsPerBeat,grid.getWidth()/(static_cast<double>(last-first)/editing::ppq+0.5));
    rowHeight=juce::jlimit(editing::minRowHeight,editing::maxRowHeight,grid.getHeight()/std::max(12,high-low+5));
    pitchOffset=std::max(0,127-high-2);updateRanges();repaint();
}
void PianoRoll::sliceNotes()
{
    auto* c=model.clip(model.active);if(!c)return;
    std::vector<MidiNote> tails;
    for(auto& note:c->notes)
    {
        const auto bounds=noteBounds(note);const double y=bounds.getCentreY();
        if(y<std::min(anchor.y,cursor.y)||y>std::max(anchor.y,cursor.y))continue;
        const double fraction=cursor.y==anchor.y?0.0:(y-anchor.y)/(cursor.y-anchor.y);
        const Tick cut=timeAt(juce::roundToInt(anchor.x+fraction*(cursor.x-anchor.x)));
        if(cut-note.start<editing::minimumNote||note.start+note.length-cut<editing::minimumNote)continue;
        if(c->notes.size()+tails.size()>=editing::maximumNotes)break;
        auto tail=note;tail.id=model.freshNoteId();tail.start=cut;tail.length=note.start+note.length-cut;
        note.length=cut-note.start;tails.push_back(tail);
    }
    c->notes.insert(c->notes.end(),tails.begin(),tails.end());model.changed();
}
void PianoRoll::transform(int operation)
{
    editNotes(model,selected,static_cast<NoteEdit>(operation),static_cast<unsigned>(juce::Time::getMillisecondCounter()));
    grabKeyboardFocus();
}
void PianoRoll::showActions()
{
    juce::PopupMenu menu;
    menu.addSectionHeader("Selected notes (or all if none selected)");
    menu.addItem(1,"Quick legato (Ctrl+L)");menu.addItem(2,"Glue touching notes (Ctrl+G)");
    menu.addItem(3,"Chop to grid (Ctrl+U)");menu.addItem(4,"Reverse time (Alt+Y)");
    menu.addItem(5,"Flip pitch");menu.addItem(6,"Strum chord, 15 ticks/note (Alt+S)");
    menu.addItem(7,"Arpeggiate chord to grid (Alt+A)");menu.addItem(8,"Humanize timing / velocity (Alt+R)");
    menu.addSeparator();menu.addItem(9,"Fit pitches to C major");menu.addItem(10,"Fit pitches to C minor");
    menu.addItem(11,"Build major triads");menu.addItem(12,"Build minor triads");
    menu.addSeparator();menu.addItem(100,"Ghost clips (Alt+V)",true,showGhosts);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&actions),[safe=juce::Component::SafePointer<PianoRoll>(this)](int id)
    {
        if(!safe||!id)return;
        if(id==100){safe->showGhosts=!safe->showGhosts;safe->repaint();safe->grabKeyboardFocus();}
        else safe->transform(id);
    });
}
bool PianoRoll::keyPressed(const juce::KeyPress& key)
{
    if(computerKeyboardMode&&!key.getModifiers().isAnyModifierKeyDown()&&key.getKeyCode()>=65&&key.getKeyCode()<=90)return false;
    for(auto* focus=juce::Component::getCurrentlyFocusedComponent();focus&&focus!=this;focus=focus->getParentComponent())
        if(dynamic_cast<juce::TextEditor*>(focus))return false;
    const EditorKey input(key);const int code=input.code;const bool ctrl=input.control;
    const bool alt=key.getModifiers().isAltDown(),shiftDown=key.getModifiers().isShiftDown();
    if(alt&&code=='V'){showGhosts=!showGhosts;repaint();return true;}
    if(code==juce::KeyPress::returnKey&&!selected.empty()){noteProperties(*selected.begin());return true;}
    if(ctrl&&shiftDown&&code=='M'){if(onCreateClip)onCreateClip();return true;}
    if(ctrl&&(code=='Z'||code=='Y')){if(code=='Y'||shiftDown)model.redo();else model.undo();return true;}
    if(ctrl&&(code=='L'||code=='G'||code=='U')){transform(code=='L'?1:code=='G'?2:3);return true;}
    if(alt&&(code=='Y'||code=='S'||code=='A'||code=='R')){transform(code=='Y'?4:code=='S'?6:code=='A'?7:8);return true;}
    if(shiftDown&&code=='I'){if(auto* c=model.clip(model.active)){for(const auto& n:c->notes){if(selected.count(n.id))selected.erase(n.id);else selected.insert(n.id);}repaint();}return true;}
    if(shiftDown&&code>='1'&&code<='5')
    {
        if(code=='4'){const auto old=selected;selected.clear();fitNotes();selected=old;}
        else if(code=='5')fitNotes();
        else{pixels=editing::pixelsPerBeat*(1<<(code-'1'));updateRanges();repaint();}
        return true;
    }
    if(code==juce::KeyPress::pageUpKey||code==juce::KeyPress::pageDownKey){zoom(code==juce::KeyPress::pageUpKey?1.25:0.8);return true;}
    if(code==juce::KeyPress::backspaceKey){model.snapIndex=model.snapIndex==0?1:0;model.changed();return true;}
    if(!ctrl&&code=='F'){property.setSelectedId(property.getSelectedId()%6+1);return true;}
    if(!ctrl&&!alt){const juce::String keys="PBEDCTZ";const int index=keys.indexOfChar(static_cast<juce::juce_wchar>(code));if(index>=0){setTool(index+1);return true;}}
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
        auto copy=clipboard;if(code=='B'){copy.clear();for(const auto& n:c->notes)if(chosen(n))copy.push_back(n);}
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



