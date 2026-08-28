#include "Arrangement.h"
#include <cmath>
namespace auralis
{
TrackRow::TrackRow(MixerState& state, TrackId track)
    : info(*state.find(track)), model(state), index(track), mute(state, track),
      gain("Track gain", "Shared arrangement/mixer gain, -60 to +6 dB. Drag vertically or type a value. Double-click resets to 0 dB. Audio-track gain controls arrangement audio; Loaded MIDI instruments use the same channel controls.", design::minimumGainDb, design::maximumGainDb, 0, info.tint)
{
    addMouseListener(this,true);
    configureButton(nameButton, juce::String(info.name), "Select this shared arrangement/mixer track and its device chain. Double-click the name or press F2 to rename both views.");
    nameButton.onClick = [this] { model.select(index); };
    configureButton(arm, "R", "Arm this shared track - UI only", true);
    arm.setColour(juce::TextButton::buttonOnColourId, colour(design::colour::coral));
    arm.onClick = [this] { model.toggleArm(index); };
    for (juce::Component* c : std::initializer_list<juce::Component*>{&nameButton, &mute, &arm, &gain}) addAndMakeVisible(c);
    gain.setTextValueSuffix(" dB");
    gain.setTitle(juce::String(info.name) + " gain");
    gain.onValueChange = [this] { model.gain(index, gain.getValue()); };
    refresh();
}
void TrackRow::refresh()
{
    nameButton.setButtonText(info.name);
    selected = model.selectedId() == index;
    gain.setValue(info.gain, juce::dontSendNotification);
    mute.refresh();
    arm.setToggleState(info.armed, juce::dontSendNotification);
    repaint();
}
void TrackRow::resized()
{
    nameButton.setBounds(12, 6, headerWidth-27, 23);
    mute.setBounds(14, 39, 28, 28);
    arm.setBounds(47, 41, 25, 24);
    gain.setBounds(headerWidth-103, 30, 78, 67);
}
HelpContent TrackRow::helpAt(juce::Point<int> point, bool) const
{
    if (point.x >= headerWidth) return {"Arrangement lane / " + juce::String(info.name), "Double-click an empty instrument lane with either mouse button to create an independent MIDI clip. Double-left-click a clip to open its piano roll. Drag library audio files onto the Audio lane; click an audio clip to edit below."};
    return {"Track header / " + juce::String(info.name), "Select this track to inspect its device path. Audio-track and master gain/mute/solo affect audio playback. Add Prism or Atlas for MIDI playback. Meters show post-fader sample peaks in dBFS. Recording remains unavailable."};
}
void TrackRow::mouseDoubleClick(const juce::MouseEvent& e)
{if(e.getEventRelativeTo(this).y<32&&model.onRenameRequested)model.onRenameRequested(index);}
void TrackRow::mouseDown(const juce::MouseEvent&) { model.select(index); }
void TrackRow::paint(juce::Graphics& g)
{
    g.fillAll(colour(design::colour::panel));
    g.setColour(colour(selected ? design::colour::raised : design::colour::panel)); g.fillRect(0, 0, headerWidth, getHeight());
    g.setColour(colour(info.tint)); g.fillRoundedRectangle(4, 12, 3, 76, 1.5f);
    text(g, juce::String(info.type), {15, 72, 70, 17}, 9, design::colour::muted, true);
    text(g, meterText(info.peakLeft,info.peakRight)+" dBFS", {15, 87, 70, 12}, 9, design::colour::muted);
    drawMeter(g,{headerWidth-13,11,10,78},info.tint,info.peakLeft,info.peakRight);
    auto timeline = juce::Rectangle<int>(headerWidth+design::panelGap, 0, getWidth()-headerWidth-design::panelGap, getHeight());
    juce::Graphics::ScopedSaveState save(g); g.reduceClipRegion(timeline);
    g.setColour(colour(selected ? design::colour::selectedLane : design::colour::panel)); g.fillRect(timeline);
    const int first = static_cast<int>(std::floor(offsetBars));
    const int last = first + timeline.getWidth()/barWidth + 2;
    for (int bar = first; bar <= last; ++bar)
    {
        const int x = timeline.getX()+juce::roundToInt((bar-offsetBars)*barWidth);
        if (bar % 4 == 0) { g.setColour(juce::Colours::white.withAlpha(0.014f)); g.fillRect(x, 0, barWidth, getHeight()); }
        g.setColour(colour(design::colour::line).withAlpha(0.7f)); g.drawVerticalLine(x, 0, static_cast<float>(getHeight()));
        for (int beat = 1; beat < beatsPerBar; ++beat)
        {
            g.setColour(colour(design::colour::line).withAlpha(0.25f));
            g.drawVerticalLine(x+barWidth*beat/beatsPerBar, 0, static_cast<float>(getHeight()));
        }
    }

    const float px = static_cast<float>(timeline.getX()+(playheadBars-offsetBars)*barWidth);
    g.setColour(colour(design::colour::mint)); g.drawLine(px, 0, px, static_cast<float>(getHeight()), 1.2f);
    g.setColour(colour(design::colour::line)); g.drawHorizontalLine(getHeight()-1, 0, static_cast<float>(getWidth()));
}
Arrangement::Arrangement(MixerState& state, MidiProject& document) : midi(document), clips(document), dropZone(document), model(state)
{
    setWantsKeyboardFocus(true);
    setHelp(*this, "Arrangement / bar ruler", "Drag the ruler to set the transport loop range. Drag its edges to resize or its top bar to move; Shift-drag sets a new range. Focus the ruler and use arrows to move, Ctrl+Left/Right to resize, Ctrl+Up/Down to double/halve. Ctrl+L loops a selected time range. Ctrl+wheel or Page Up/Down zooms; Shift+wheel scrolls time; middle-drag pans. Drop audio or a built-in instrument below the lanes to create a linked track.");
    setHelp(horizontal, "Timeline scroll", "Drag to view earlier or later bars. This changes the visible range, not the transport position.");
    setHelp(viewport.getVerticalScrollBar(), "Track scroll", "Scroll vertically to reveal tracks outside the visible arrangement area.");
    for (const auto& channel : model.all())
        if (channel.kind == ChannelKind::arrangement)
        {
            auto row = std::make_unique<TrackRow>(model, channel.id);
            canvas.addAndMakeVisible(*row);
            rows.push_back(std::move(row));
        }
    canvas.addAndMakeVisible(clips);clips.showDropHint=false;
    dropZone.laneIds.clear();addAndMakeVisible(dropZone);
    dropZone.onDrop=[this](const juce::String& item,int,Tick time){if(onDrop)onDrop(item,-1,time);};
    clips.onDrop=[this](const juce::String& item,int track,Tick time){if(onDrop)onDrop(item,track,time);};
    clips.onTrackSelect=[this](int id){if(id>0)model.select(id);};
    clips.onOpen = [this](int id) { if(onOpen) onOpen(id); };
    clips.onAudioOpen=[this](int id){if(onAudioOpen)onAudioOpen(id);};
    clips.onImport=[this](const juce::File& file,Tick time){if(onImport)onImport(file,time);};
    clips.onLoop = [this](Tick start,Tick end) { if(onLoop) onLoop(start,end); };
    clips.onZoom = [this](double factor) { barWidth=juce::jlimit(32,1280,juce::roundToInt(barWidth*factor)); resized(); };
    clips.onZoomRange=[this](Tick start,Tick end)
    {
        const double bars=static_cast<double>(std::max<Tick>(editing::minimumNote,end-start))/midi.barTicks;
        barWidth=juce::jlimit(32,1280,juce::roundToInt(std::max(1,clips.getWidth()-20)/bars));
        offsetBars=static_cast<double>(start)/midi.barTicks;resized();
    };
    clips.onVerticalZoom=[this](double factor){rowHeight=juce::jlimit(design::trackHeight,design::trackHeight*2,juce::roundToInt(rowHeight*factor));resized();};
    clips.onScroll = [this](double delta) { horizontal.setCurrentRangeStart(offsetBars+delta); };
    clips.onVerticalScroll = [this](int delta) { viewport.setViewPosition(0,viewport.getViewPositionY()+delta); };
    viewport.setViewedComponent(&canvas, false); viewport.setScrollBarsShown(true, false);
    viewport.setScrollBarThickness(9); viewport.setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::never); addAndMakeVisible(viewport);
    horizontal.addListener(this); horizontal.setAutoHide(false); addAndMakeVisible(horizontal); addAndMakeVisible(trackDivider);
    trackDivider.onDrag = [this](int delta)
    { headerWidth = juce::jlimit(design::trackMin, juce::jmin(design::trackMax, getWidth()-300), headerWidth+delta); resized(); };
    trackDivider.onReset = [this] { headerWidth = design::trackWidth; resized(); };
    configureButton(addTrack, "+ MIDI clip", "Ctrl+Shift+M: create a MIDI clip at the last timeline cursor. Double-click an empty instrument lane with either mouse button to create and open a clip. Audio tracks do not accept MIDI clips.");
    addTrack.onClick=[this]{createClip();};
    configureButton(automationToggle,"Automation","Shift+A: show grouped automation lanes. Move a channel or device knob to add its lane; Alt-drag curve segments to bend them.",true);
    automationToggle.onClick=[this]{toggleAutomation();};addAndMakeVisible(automationToggle);
    addAndMakeVisible(addTrack); refresh();
}
void Arrangement::refresh()
{
    automationToggle.setToggleState(model.automationMode,juce::dontSendNotification);
    bool appended=false;clips.laneIds.clear();midi.instrumentTracks.clear();
    for(const auto& c:model.all())if(c.kind==ChannelKind::arrangement)
    {
        clips.laneIds.push_back(c.id);if(c.type=="MIDI")midi.instrumentTracks.push_back(c.id);
        if(std::none_of(rows.begin(),rows.end(),[&](const auto& r){return r->info.id==c.id;}))
        {auto row=std::make_unique<TrackRow>(model,c.id);canvas.addAndMakeVisible(*row);rows.push_back(std::move(row));appended=true;}
    }
    for(const auto& c:model.all())for(const auto& lane:c.automation)
        if(std::none_of(automationRows.begin(),automationRows.end(),[&](const auto& row){return row->channelId==c.id&&row->laneId==lane.id;}))
        {auto row=std::make_unique<AutomationLaneView>(model,midi,c.id,lane.id);canvas.addAndMakeVisible(*row);automationRows.push_back(std::move(row));appended=true;}
    if(appended)resized();
    for(auto& row:automationRows){row->setVisible(model.automationMode);row->refresh();}
    for(auto& row:rows)row->refresh();clips.repaint();repaint();
}
void Arrangement::mouseMove(const juce::MouseEvent& e)
{
    auto pointerStyle=juce::MouseCursor::NormalCursor;
    if(e.x>headerWidth+design::panelGap&&e.y<design::rulerHeight)
    {
        pointerStyle=juce::MouseCursor::CrosshairCursor;
        const double left=headerWidth+design::panelGap+(static_cast<double>(loopStart)/midi.barTicks-offsetBars)*barWidth;
        const double right=headerWidth+design::panelGap+(static_cast<double>(loopEnd)/midi.barTicks-offsetBars)*barWidth;
        if(loopEnabled&&(std::abs(e.x-left)<=6||std::abs(e.x-right)<=6))pointerStyle=juce::MouseCursor::LeftRightResizeCursor;
        else if(loopEnabled&&e.x>left&&e.x<right&&e.y<14)pointerStyle=juce::MouseCursor::DraggingHandCursor;
    }
    setMouseCursor(pointerStyle);
}
void Arrangement::resetLayout() { headerWidth = design::trackWidth; barWidth = design::defaultBarWidth; horizontal.setCurrentRangeStart(0); resized(); }
void Arrangement::setPlayhead(double position, int numerator)
{
    if (playhead == position && beats == numerator) return;
    playhead = position; beats = numerator; updateRows();
}
void Arrangement::updateRows()
{
    for (auto& row : rows)
    {
        row->headerWidth = headerWidth; row->offsetBars = offsetBars; row->barWidth = barWidth;
        row->playheadBars = playhead; row->beatsPerBar = beats; row->resized(); row->repaint();
    }
    for(auto& row:automationRows){row->offsetBars=offsetBars;row->barWidth=barWidth;row->headerWidth=headerWidth;row->resized();row->repaint();}
    clips.offsetBars=offsetBars; clips.barWidth=barWidth; clips.rowHeight=rowHeight; clips.repaint();
    repaint();
}
void Arrangement::scrollBarMoved(juce::ScrollBar*, double start) { offsetBars = start; updateRows(); }
void Arrangement::updateScrollRange()
{
    const double visible = static_cast<double>(juce::jmax(1, viewport.getMaximumVisibleWidth()-headerWidth-design::panelGap))/barWidth;
    horizontal.setRangeLimits(0, design::timelineBars);
    horizontal.setCurrentRange(offsetBars, juce::jmin(visible, static_cast<double>(design::timelineBars)), juce::dontSendNotification);
    offsetBars = horizontal.getCurrentRangeStart();
}
void Arrangement::resized()
{
    addTrack.setBounds(12,5,(headerWidth-24)/2,26);automationToggle.setBounds(headerWidth/2,5,headerWidth/2-12,26);
    viewport.setBounds(0, design::arrangementToolbar+design::rulerHeight, getWidth(), getHeight()-design::arrangementToolbar-design::rulerHeight-24-devices::dropHeight);
    int totalHeight=static_cast<int>(rows.size())*rowHeight;
    if(model.automationMode)totalHeight+=static_cast<int>(automationRows.size())*84;
    canvas.setSize(viewport.getMaximumVisibleWidth(),std::max(viewport.getHeight(),totalHeight+devices::dropHeight));
    clips.laneTops.clear();int y=0;
    for(auto& row:rows)
    {
        clips.laneTops.push_back(y);row->setBounds(0,y,canvas.getWidth(),rowHeight);y+=rowHeight;
        if(model.automationMode)for(auto& lane:automationRows)if(lane->channelId==row->info.id)
        {lane->setBounds(0,y,canvas.getWidth(),84);y+=84;lane->toFront(false);}
    }
    if(model.automationMode)for(const auto& channel:model.all())if(channel.kind!=ChannelKind::arrangement)
        for(auto& lane:automationRows)if(lane->channelId==channel.id)
        {lane->setBounds(0,y,canvas.getWidth(),84);y+=84;lane->toFront(false);}
    dropZone.setBounds(headerWidth+design::panelGap,getHeight()-24-devices::dropHeight,getWidth()-headerWidth-design::panelGap,devices::dropHeight);
    dropZone.offsetBars=offsetBars;dropZone.barWidth=barWidth;
    clips.setBounds(headerWidth+design::panelGap,0,canvas.getWidth()-headerWidth-design::panelGap,canvas.getHeight());
    trackDivider.setBounds(headerWidth, design::arrangementToolbar, design::panelGap, getHeight()-design::arrangementToolbar-24);
    horizontal.setBounds(headerWidth+design::panelGap, getHeight()-18, getWidth()-headerWidth-design::panelGap-12, 10);
    clips.toFront(false);
    if(model.automationMode)for(auto& lane:automationRows)lane->toFront(false);
    updateScrollRange(); updateRows();
}
void Arrangement::paint(juce::Graphics& g)
{
    panel(g, getLocalBounds());



    const int left = headerWidth+design::panelGap;
    juce::Graphics::ScopedSaveState save(g); g.reduceClipRegion(left, design::arrangementToolbar, getWidth()-left-9, design::rulerHeight);
    g.setColour(colour(design::colour::background)); g.fillRect(left, design::arrangementToolbar, getWidth()-left, design::rulerHeight);
    for (int bar = static_cast<int>(offsetBars); bar < offsetBars + static_cast<double>(getWidth()-left)/barWidth+1; ++bar)
    {
        const int x = left+juce::roundToInt((bar-offsetBars)*barWidth);
        text(g, juce::String(bar+1), {x+9, design::arrangementToolbar+7, 38, 22}, 11, design::colour::muted, true);
        if (loopEnabled && bar*midi.barTicks >= loopStart && bar*midi.barTicks < loopEnd) { g.setColour(colour(design::colour::mint).withAlpha(0.35f)); g.fillRect(x, design::arrangementToolbar, barWidth, 2); }
    }
    if(loopEnabled)
    {
        const float x1=static_cast<float>(left+(static_cast<double>(loopStart)/midi.barTicks-offsetBars)*barWidth);
        const float x2=static_cast<float>(left+(static_cast<double>(loopEnd)/midi.barTicks-offsetBars)*barWidth);
        g.setColour(colour(design::colour::mint).withAlpha(0.22f));g.fillRect(x1,0.0f,x2-x1,static_cast<float>(design::rulerHeight));
        g.setColour(colour(design::colour::mint));g.drawLine(x1,3,x2,3,3);g.drawLine(x1,3,x1,13,2);g.drawLine(x2,3,x2,13,2);
    }
    const float px = static_cast<float>(left+(playhead-offsetBars)*barWidth);
    juce::Path marker; marker.addTriangle(px-5, 24, px+5, 24, px, 32);
    g.setColour(colour(design::colour::mint)); g.fillPath(marker);
}
void Arrangement::setLoop(Tick start,Tick end,bool enabled)
{
    if(draggingLoop)return;
    if(loopStart==start&&loopEnd==end&&loopEnabled==enabled)return;
    loopStart=start;loopEnd=end;loopEnabled=enabled;repaint();
}
void Arrangement::mouseDown(const juce::MouseEvent& e)
{
    if(e.y>=design::rulerHeight||e.x<headerWidth+design::panelGap)return;
    grabKeyboardFocus();
    loopAnchor=midi.snap(static_cast<Tick>((offsetBars+static_cast<double>(e.x-headerWidth-design::panelGap)/barWidth)*midi.barTicks));
    originalLoopStart=loopStart;originalLoopEnd=loopEnd;
    const double startX=headerWidth+design::panelGap+(static_cast<double>(loopStart)/midi.barTicks-offsetBars)*barWidth;
    const double endX=headerWidth+design::panelGap+(static_cast<double>(loopEnd)/midi.barTicks-offsetBars)*barWidth;
    loopDrag=LoopDrag::create;
    if(loopEnabled&&!e.mods.isShiftDown())
    {
        if(std::abs(e.x-startX)<=6)loopDrag=LoopDrag::start;
        else if(std::abs(e.x-endX)<=6)loopDrag=LoopDrag::end;
        else if(e.y<14&&loopAnchor>=loopStart&&loopAnchor<loopEnd)loopDrag=LoopDrag::move;
    }
    draggingLoop=true;
}
void Arrangement::mouseDrag(const juce::MouseEvent& e)
{
    if(!draggingLoop)return;
    const Tick end=midi.snap(static_cast<Tick>((offsetBars+static_cast<double>(e.x-headerWidth-design::panelGap)/barWidth)*midi.barTicks));
    if(loopDrag==LoopDrag::start)loopStart=std::min(end,loopEnd-editing::minimumNote);
    else if(loopDrag==LoopDrag::end)loopEnd=std::max(end,loopStart+editing::minimumNote);
    else if(loopDrag==LoopDrag::move)
    {
        const Tick delta=std::clamp<Tick>(end-loopAnchor,-originalLoopStart,editing::maximumTime-originalLoopEnd);
        loopStart=originalLoopStart+delta;loopEnd=originalLoopEnd+delta;
    }
    else{loopStart=std::min(loopAnchor,end);loopEnd=std::max(loopAnchor,end);}
    loopEnabled=true;repaint();
}
void Arrangement::mouseUp(const juce::MouseEvent&)
{
    if(draggingLoop&&loopEnd>loopStart&&onLoop)onLoop(loopStart,loopEnd);
    draggingLoop=false;
}
bool Arrangement::keyPressed(const juce::KeyPress& key)
{
    if(!hasKeyboardFocus(false)||!loopEnabled)return false;
    const int code=key.getKeyCode();
    if(code!=juce::KeyPress::leftKey&&code!=juce::KeyPress::rightKey&&code!=juce::KeyPress::upKey&&code!=juce::KeyPress::downKey)return false;
    const bool positive=code==juce::KeyPress::rightKey||code==juce::KeyPress::downKey;
    const Tick length=loopEnd-loopStart;
    const Tick step=std::max(editing::minimumNote,midi.step());
    if(key.getModifiers().isCtrlDown())
    {
        Tick next=length;
        if(code==juce::KeyPress::upKey)next=length*2;
        else if(code==juce::KeyPress::downKey)next=length/2;
        else next+=positive?step:-step;
        loopEnd=loopStart+std::clamp<Tick>(next,editing::minimumNote,editing::maximumTime-loopStart);
    }
    else
    {
        const Tick amount=code==juce::KeyPress::upKey||code==juce::KeyPress::downKey?length:step;
        const Tick delta=std::clamp<Tick>(positive?amount:-amount,-loopStart,editing::maximumTime-loopEnd);
        loopStart+=delta;loopEnd+=delta;
    }
    if(onLoop)onLoop(loopStart,loopEnd);repaint();return true;
}

}




