#include "Arrangement.h"
#include <cmath>
namespace auralis
{
TrackRow::TrackRow(MixerState& state, TrackId track)
    : info(*state.find(track)), model(state), index(track), mute(state, track),
      gain("Track gain", "Shared arrangement/mixer gain, -60 to +6 dB. Drag vertically or type a value. Double-click resets to 0 dB. UI state only.", design::minimumGainDb, design::maximumGainDb, 0, info.tint)
{
    configureButton(nameButton, juce::String(info.name), "Select this shared arrangement/mixer track and its device chain");
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
    if (point.x >= headerWidth) return {"Arrangement lane / " + juce::String(info.name), "Double-right-click an instrument lane to create an independent MIDI clip. Double-left-click a clip to open its piano roll. Audio import is not implemented."};
    return {"Track header / " + juce::String(info.name), "Select this track to inspect its device path. Mute, solo, arm and gain are UI state only. The stereo meter is silent (-infinity dB); no audio is processed."};
}
void TrackRow::mouseDown(const juce::MouseEvent&) { model.select(index); }
void TrackRow::paint(juce::Graphics& g)
{
    g.fillAll(colour(design::colour::panel));
    g.setColour(colour(selected ? design::colour::raised : design::colour::panel)); g.fillRect(0, 0, headerWidth, getHeight());
    g.setColour(colour(info.tint)); g.fillRoundedRectangle(4, 12, 3, 76, 1.5f);
    text(g, juce::String(info.type), {15, 72, 70, 17}, 9, design::colour::muted, true);
    text(g, "-inf dB", {15, 87, 70, 12}, 9, design::colour::muted);
    // Deliberately silent until an audio source exists.
    g.setColour(colour(design::colour::background)); g.fillRoundedRectangle(static_cast<float>(headerWidth-13), 11, 10, 78, 2);
    g.setColour(colour(design::colour::line));
    for (int segment = 0; segment < 12; ++segment)
    {
        g.fillRect(headerWidth-12, 13+segment*6, 3, 4);
        g.fillRect(headerWidth-7, 13+segment*6, 3, 4);
    }
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
Arrangement::Arrangement(MixerState& state, MidiProject& document) : midi(document), clips(document), model(state)
{
    setHelp(*this, "Arrangement / bar ruler", "Drag the ruler to set the transport loop range. Ctrl+L loops a selected time range. Ctrl+wheel or Page Up/Down zooms; Shift+wheel scrolls time; middle-drag pans. Adding tracks is not implemented.");
    setHelp(horizontal, "Timeline scroll", "Drag to view earlier or later bars. This changes the visible range, not the transport position.");
    setHelp(viewport.getVerticalScrollBar(), "Track scroll", "Scroll vertically to reveal tracks outside the visible arrangement area.");
    for (const auto& channel : model.all())
        if (channel.kind == ChannelKind::arrangement)
        {
            auto row = std::make_unique<TrackRow>(model, channel.id);
            canvas.addAndMakeVisible(*row);
            rows.push_back(std::move(row));
        }
    canvas.addAndMakeVisible(clips);
    clips.onOpen = [this](int id) { if(onOpen) onOpen(id); };
    clips.onLoop = [this](Tick start,Tick end) { if(onLoop) onLoop(start,end); };
    clips.onZoom = [this](double factor) { barWidth=juce::jlimit(32,1280,juce::roundToInt(barWidth*factor)); resized(); };
    clips.onVerticalZoom=[this](double factor){rowHeight=juce::jlimit(design::trackHeight,design::trackHeight*2,juce::roundToInt(rowHeight*factor));resized();};
    clips.onScroll = [this](double delta) { horizontal.setCurrentRangeStart(offsetBars+delta); };
    clips.onVerticalScroll = [this](int delta) { viewport.setViewPosition(0,viewport.getViewPositionY()+delta); };
    viewport.setViewedComponent(&canvas, false); viewport.setScrollBarsShown(true, false);
    viewport.setScrollBarThickness(9); viewport.setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::never); addAndMakeVisible(viewport);
    horizontal.addListener(this); horizontal.setAutoHide(false); addAndMakeVisible(horizontal); addAndMakeVisible(trackDivider);
    trackDivider.onDrag = [this](int delta)
    { headerWidth = juce::jlimit(design::trackMin, juce::jmin(design::trackMax, getWidth()-300), headerWidth+delta); resized(); };
    trackDivider.onReset = [this] { headerWidth = design::trackWidth; resized(); };
    configureButton(addTrack, "+ Track", "Adding tracks will be implemented in a future milestone");
    addTrack.setEnabled(false); addAndMakeVisible(addTrack); refresh();
}
void Arrangement::refresh() { for (auto& row : rows) row->refresh(); clips.repaint(); repaint(); }
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
    addTrack.setBounds(16, 5, headerWidth-28, 26);
    viewport.setBounds(0, design::arrangementToolbar+design::rulerHeight, getWidth(), getHeight()-design::arrangementToolbar-design::rulerHeight-24);
    canvas.setSize(viewport.getMaximumVisibleWidth(), juce::jmax(viewport.getHeight(), static_cast<int>(rows.size())*rowHeight));
    for (size_t i = 0; i < rows.size(); ++i) rows[i]->setBounds(0, static_cast<int>(i)*rowHeight, canvas.getWidth(), rowHeight);
    clips.setBounds(headerWidth+design::panelGap,0,canvas.getWidth()-headerWidth-design::panelGap,canvas.getHeight());
    trackDivider.setBounds(headerWidth, design::arrangementToolbar, design::panelGap, getHeight()-design::arrangementToolbar-24);
    horizontal.setBounds(headerWidth+design::panelGap, getHeight()-18, getWidth()-headerWidth-design::panelGap-12, 10);
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
    loopAnchor=midi.snap(static_cast<Tick>((offsetBars+static_cast<double>(e.x-headerWidth-design::panelGap)/barWidth)*midi.barTicks));
    draggingLoop=true;
}
void Arrangement::mouseDrag(const juce::MouseEvent& e)
{
    if(!draggingLoop)return;
    const Tick end=midi.snap(static_cast<Tick>((offsetBars+static_cast<double>(e.x-headerWidth-design::panelGap)/barWidth)*midi.barTicks));
    loopStart=std::min(loopAnchor,end);loopEnd=std::max(loopAnchor,end);loopEnabled=true;repaint();
}
void Arrangement::mouseUp(const juce::MouseEvent&)
{
    if(draggingLoop&&loopEnd>loopStart&&onLoop)onLoop(loopStart,loopEnd);draggingLoop=false;
}

}


