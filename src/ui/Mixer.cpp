#include "Mixer.h"
namespace auralis
{
MixerStrip::MixerStrip(MixerState& state, TrackId track)
    : model(state), id(track), mute(state, track),
      pan("Panning", "Stereo balance: -100 left, 0 center, +100 right. Double-click for center. UI state only.", mixing::panMinimum, mixing::panMaximum, 0),
      stereo("Stereo separation / merge", "Turn left for separation (-100), center for original stereo (0), right to merge to mono (+100). Double-click for original stereo. UI state only.", mixing::stereoMinimum, mixing::stereoMaximum, 0),
      amount("Send amount", "Level of the selected source sent to this destination, 0 to 100 percent. Double-click restores unity (100%). This is routing state, not live audio.", 0, 100, 100, design::colour::amber)
{
    const auto& c = *model.find(id);
    configureButton(select, juce::String(c.name), "Select " + juce::String(c.name) + " as the routing source and inspect its shared device chain.");
    select.onClick = [this] { model.select(id); };
    polarity.onClick = [this] { model.togglePolarity(id); };
    pan.onValueChange = [this] { model.pan(id, pan.getValue()); };
    stereo.onValueChange = [this] { model.stereo(id, stereo.getValue()); };
    pan.textFromValueFunction = [](double v) { return std::abs(v) < 0.05 ? juce::String("C") : juce::String(std::abs(v), 0)+(v < 0 ? " L" : " R"); };
    stereo.textFromValueFunction = [](double v) { return std::abs(v) < 0.05 ? juce::String("Original") : juce::String(std::abs(v), 0)+(v < 0 ? "% sep" : "% mono"); };
    pan.setTextBoxIsEditable(false); stereo.setTextBoxIsEditable(false);
    pan.updateText(); stereo.updateText();
    amount.setTextValueSuffix(" %");
    amount.onValueChange = [this] { model.sendAmount(model.selectedId(), id, amount.getValue()/100); };
    routeButton.onClick = [this] { model.toggleRoute(model.selectedId(), id); };
    gain.setSliderStyle(juce::Slider::LinearVertical);
    gain.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 78, 19);
    gain.setRange(design::minimumGainDb, design::maximumGainDb, 0.1);
    gain.setSkewFactorFromMidPoint(-12);
    gain.setDoubleClickReturnValue(true, 0);
    gain.setTextValueSuffix(" dB"); gain.setNumDecimalPlacesToDisplay(1);
    gain.setColour(juce::Slider::thumbColourId, colour(c.tint));
    setHelp(gain, juce::String(c.name)+" gain", "Shared channel gain, -60 to +6 dB. Drag the fader or type a value; double-click resets 0 dB. Arrangement and mixer use the same value. No audio engine yet.");
    gain.onValueChange = [this] { model.gain(id, gain.getValue()); };
    setHelp(*this, juce::String(c.name)+" channel", "Select this strip to display its outgoing routing cables. Meters remain at -infinity dBFS until audio exists. Mute, solo, gain, pan, polarity and stereo width are shared UI state.");
    for (juce::Component* control : std::initializer_list<juce::Component*>{&select,&mute,&polarity,&pan,&stereo,&gain,&routeButton,&amount}) addAndMakeVisible(control);
    refresh();
}
void MixerStrip::refresh()
{
    const auto& c = *model.find(id);
    gain.setValue(c.gain, juce::dontSendNotification);
    pan.setValue(c.pan, juce::dontSendNotification);
    stereo.setValue(c.stereo, juce::dontSendNotification);
    polarity.setToggleState(c.polarity, juce::dontSendNotification);
    mute.refresh();
    const auto* route = model.route(model.selectedId(), id);
    routeButton.setToggleState(route != nullptr, juce::dontSendNotification);
    routeButton.setEnabled(route != nullptr || model.canRoute(model.selectedId(), id));
    routeButton.setTitle("Send " + juce::String(model.selectedChannel().name) + " to " + juce::String(c.name));
    amount.setVisible(route != nullptr);
    amount.setValue(route ? route->amount*100 : 0, juce::dontSendNotification);
    repaint();
}
void MixerStrip::resized()
{
    select.setBounds(4, 8, getWidth()-8, 28);
    mute.setBounds(8, 42, 28, 28);
    polarity.setBounds(getWidth()-36, 42, 28, 28);
    pan.setBounds(24, 77, getWidth()-48, 68);
    const int bottom = getHeight()-mixing::routingHeight;
    stereo.setBounds(24, bottom-157, getWidth()-48, 66);
    gain.setBounds(33, 160, getWidth()-37, juce::jmax(35, bottom-327));
    amount.setBounds(23, bottom-85, getWidth()-46, 58);
    routeButton.setBounds(getWidth()/2-15, bottom-26, 30, 25);
}
void MixerStrip::paint(juce::Graphics& g)
{
    const auto& c = *model.find(id);
    auto r = getLocalBounds().withTrimmedBottom(mixing::routingHeight).reduced(2, 0);
    g.setColour(colour(model.selectedId() == id ? design::colour::raised : design::colour::panel));
    g.fillRoundedRectangle(r.toFloat(), 6);
    g.setColour(colour(c.tint)); g.fillRoundedRectangle(5, 3, static_cast<float>(getWidth()-10), 3, 1);
    text(g, "PAN", {0, 69, getWidth(), 16}, 9, design::colour::muted, false, juce::Justification::centred);
    const int bottom = getHeight()-mixing::routingHeight;
    drawSilentMeter(g, {9, 164, 18, juce::jmax(22, bottom-345)}, c.tint);
    text(g, "-inf", {4, bottom-177, 28, 16}, 9, design::colour::muted);
    text(g, "STEREO", {0, bottom-169, getWidth(), 14}, 9, design::colour::muted, false, juce::Justification::centred);
    if (model.selectedId() == id)
        text(g, "SOURCE", {0, bottom-54, getWidth(), 18}, 9, c.tint, true, juce::Justification::centred);
}
void MixerStrip::mouseDown(const juce::MouseEvent&) { model.select(id); }

MixerBody::MixerBody(MixerState& state) : model(state), master(state, 0)
{
    addAndMakeVisible(master); addAndMakeVisible(sendDivider);
    setHelp(sendDivider,"Send dock divider","Drag to resize the send dock. Double-click resets its width. Hiding the dock preserves this width and every route.");
    sendDivider.onDrag=[this](int delta){dockWidth-=delta;resized();repaint();};
    sendDivider.onReset=[this]{dockWidth=mixing::dockWidth;resized();repaint();};
    trackViewport.setViewedComponent(&trackCanvas, false);
    sendViewport.setViewedComponent(&sendCanvas, false);
    for (auto* view : {&trackViewport, &sendViewport})
    {
        view->setScrollBarsShown(false, true);
        view->setScrollBarThickness(10);
        view->setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::never);
        addAndMakeVisible(view);
        setHelp(view->getHorizontalScrollBar(), "Mixer channel scroll", "Scroll horizontally to reach more channel strips. Master and current meters remain docked.");
    }
    for (const auto& c : model.all()) if (c.kind == ChannelKind::arrangement)
    {
        auto strip = std::make_unique<MixerStrip>(model, c.id);
        trackCanvas.addAndMakeVisible(*strip);
        tracks.push_back(std::move(strip));
    }
    refresh();
}
MixerBody::~MixerBody()
{
    trackViewport.setViewedComponent(nullptr, false);
    sendViewport.setViewedComponent(nullptr, false);
}
void MixerBody::revealLastSend()
{
    sendViewport.setViewPosition(juce::jmax(0, sendCanvas.getWidth()-sendViewport.getWidth()), 0);
}
void MixerBody::refresh()
{
    // Append only: do not destroy the clicked strip while a callback is active.
    for (const auto& c : model.all()) if (c.kind == ChannelKind::send)
    {
        const bool exists = std::any_of(sends.begin(), sends.end(), [&](const auto& s) { return s->trackId() == c.id; });
        if (!exists)
        {
            auto strip = std::make_unique<MixerStrip>(model, c.id);
            sendCanvas.addAndMakeVisible(*strip);
            sends.push_back(std::move(strip));
        }
    }
    master.refresh();
    for (auto& s : tracks) s->refresh();
    for (auto& s : sends) s->refresh();
    juce::String help = "Selected source: " + juce::String(model.selectedChannel().name) + ". Outgoing sends: ";
    for (const auto& r : model.sends()) if (r.source == model.selectedId())
        help += juce::String(model.find(r.destination)->name) + " (" + juce::String(r.amount*100, 0) + "%). ";
    setHelp(*this, "Current meter / routing", help + "Meters are silent dBFS placeholders. Hidden destinations remain connected; show the send dock or scroll channel lanes to inspect them.");
    resized(); repaint();
}
void MixerBody::resized()
{
    auto area = getLocalBounds();
    area.removeFromLeft(mixing::currentWidth);
    master.setBounds(area.removeFromLeft(mixing::stripWidth));
    if (showSends)
    {
        dockWidth = juce::jlimit(mixing::stripWidth,juce::jmax(mixing::stripWidth,area.getWidth()-mixing::stripWidth-design::panelGap),dockWidth);
        const int dockSize = dockWidth;
        auto dockArea = area.removeFromRight(dockSize);
        sendViewport.setBounds(dockArea);
        sendDivider.setBounds(area.removeFromRight(design::panelGap));
    }
    sendViewport.setVisible(showSends); sendDivider.setVisible(showSends);
    trackViewport.setBounds(area);
    trackCanvas.setSize(static_cast<int>(tracks.size())*mixing::stripWidth, getHeight()-12);
    sendCanvas.setSize(juce::jmax(sendViewport.getWidth(), static_cast<int>(sends.size())*mixing::stripWidth), getHeight()-12);
    for (size_t i = 0; i < tracks.size(); ++i) tracks[i]->setBounds(static_cast<int>(i)*mixing::stripWidth, 0, mixing::stripWidth, getHeight());
    for (size_t i = 0; i < sends.size(); ++i) sends[i]->setBounds(static_cast<int>(i)*mixing::stripWidth, 0, mixing::stripWidth, getHeight());
}
void MixerBody::paint(juce::Graphics& g)
{
    const auto& current = model.selectedChannel();
    text(g, "CURRENT", {0, 9, mixing::currentWidth, 18}, 9, current.tint, true, juce::Justification::centred);
    text(g, juce::String(current.name), {3, 30, mixing::currentWidth-6, 21}, 9, design::colour::muted, false, juce::Justification::centred);
    drawSilentMeter(g, {3, 65, mixing::currentWidth-9, getHeight()-190}, current.tint, true);
    text(g, "-inf dBFS", {0, getHeight()-119, mixing::currentWidth, 18}, 9, design::colour::muted, false, juce::Justification::centred);
    if (showSends)
    {
        const int x = sendViewport.getX();
        g.setColour(colour(design::colour::line)); g.drawVerticalLine(x, 0, static_cast<float>(getHeight()));
        if (sends.empty())
        {
            text(g, "SEND TRACKS", {x+10, 24, sendViewport.getWidth()-20, 22}, 11, design::colour::coral, true);
            text(g, "Use + to create a send", {x+10, 56, sendViewport.getWidth()-20, 22}, 10, design::colour::muted);
            text(g, "Independent of arrangement", {x+10, 83, sendViewport.getWidth()-20, 22}, 9, design::colour::muted);
        }
    }
}
std::optional<juce::Point<float>> MixerBody::endpoint(TrackId id) const
{
    if (id == 0) return juce::Point<float>(static_cast<float>(master.getBounds().getCentreX()), static_cast<float>(getHeight()-mixing::routingHeight-6));
    const auto locate = [&](const auto& strips, const juce::Viewport& view) -> std::optional<juce::Point<float>>
    {
        for (const auto& strip : strips) if (strip->trackId() == id)
        {
            auto p = getLocalPoint(strip.get(), juce::Point<int>{strip->getWidth()/2, strip->getHeight()-mixing::routingHeight-6});
            if (p.x < view.getX() || p.x >= view.getRight()) return std::nullopt;
            return p.toFloat();
        }
        return std::nullopt;
    };
    if (auto point = locate(tracks, trackViewport)) return point;
    return showSends ? locate(sends, sendViewport) : std::nullopt;
}
void MixerBody::paintOverChildren(juce::Graphics& g)
{
    const auto source = endpoint(model.selectedId());
    if (!source) return;
    int index = 0;
    for (const auto& route : model.sends()) if (route.source == model.selectedId())
    {
        if (const auto destination = endpoint(route.destination))
        {
            const float depth = static_cast<float>(getHeight()-20-(index++%3)*8);
            juce::Path wire;
            wire.startNewSubPath(*source);
            wire.cubicTo(source->x, depth, destination->x, depth, destination->x, destination->y);
            g.setColour(colour(model.selectedChannel().tint).withAlpha(0.75f));
            g.strokePath(wire, juce::PathStrokeType(1.6f));
        }
    }
}
Mixer::Mixer(MixerState& state) : model(state), body(state)
{
    viewport.setViewedComponent(&body, false);
    viewport.setScrollBarsShown(true, false);
    viewport.setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::never);
    viewport.setScrollBarThickness(10);
    addAndMakeVisible(viewport); addAndMakeVisible(dock); addAndMakeVisible(add);
    dock.onClick = [this] { body.showSends = !body.showSends; refresh(); };
    add.onClick = [this] { body.showSends = true; model.addSend(); refresh(); body.revealLastSend(); };
    setHelp(*this, "Mixer", "Select a source strip, then use destination arrows and amount knobs to build post-fader routes. Send tracks live in the right dock. Meters are silent; no audio engine is connected.");
    setHelp(viewport.getVerticalScrollBar(), "Mixer vertical scroll", "Scroll down to reach stereo and routing controls when the mixer panel is short.");
    refresh();
}
Mixer::~Mixer() { viewport.setViewedComponent(nullptr, false); }
void Mixer::refresh()
{
    dock.setToggleState(body.showSends, juce::dontSendNotification);
    add.setEnabled(model.sendCount() < mixing::maximumSends);
    body.refresh(); resized(); repaint();
}
void Mixer::resetLayout()
{
    body.showSends = true; body.dockWidth=mixing::dockWidth; viewport.setViewPosition(0, 0); refresh();
}
void Mixer::resized()
{
    dock.setBounds(getWidth()-76, 3, 32, 30);
    add.setBounds(getWidth()-38, 3, 32, 30);
    viewport.setBounds(0, mixing::toolbarHeight, getWidth(), getHeight()-mixing::toolbarHeight);
    body.setSize(viewport.getMaximumVisibleWidth(), juce::jmax(mixing::minimumBodyHeight, viewport.getHeight()));
}
void Mixer::paint(juce::Graphics& g)
{
    panel(g, getLocalBounds());
    text(g, "FROM  " + juce::String(model.selectedChannel().name) + "   /   ROUTING PREVIEW", {14, 5, getWidth()-98, 25}, 10, model.selectedChannel().tint, true);
}
}

