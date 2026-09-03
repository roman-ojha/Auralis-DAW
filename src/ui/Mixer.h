#pragma once
#include "MixerControls.h"

namespace auralis
{
class RoutingViewport final : public juce::Viewport
{
public:
    void visibleAreaChanged(const juce::Rectangle<int>&) override { if (auto* parent = getParentComponent()) parent->repaint(); }
};
class MixerStrip final : public juce::Component
{
public:
    MixerStrip(MixerState&, TrackId);
    void refresh();
    void resized() override;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    TrackId trackId() const { return id; }
private:
    MixerState& model;
    TrackId id;
    juce::TextButton select;
    MuteSoloButton mute;
    IconButton polarity{ControlIcon::polarity, "Reverse polarity", "Invert this channel's polarity. Applied before the channel output and sends."};
    IconButton routeButton{ControlIcon::route, "Send destination", "Select a source strip, then click a destination arrow to connect or disconnect a post-fader send. The knob above adjusts this route. Self-routing and feedback cycles are blocked."};
    Knob pan, stereo, amount;
    juce::Slider gain;
};
class MixerBody final : public juce::Component
{
public:
    explicit MixerBody(MixerState&);
    ~MixerBody() override;
    void refresh();
    void revealLastSend();
    void clearRows(){tracks.clear();sends.clear();}
    void resized() override;
    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    bool showSends = true;
    int dockWidth = mixing::dockWidth;
private:
    MixerState& model;
    MixerStrip master;
    Splitter sendDivider{true};
    juce::Component trackCanvas, sendCanvas;
    RoutingViewport trackViewport, sendViewport;
    std::vector<std::unique_ptr<MixerStrip>> tracks, sends;
    std::optional<juce::Point<float>> endpoint(TrackId) const;
};
class Mixer final : public juce::Component
{
public:
    explicit Mixer(MixerState&);
    ~Mixer() override;
    void refresh();
    void resetLayout();
    void clearRows(){body.clearRows();}
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    MixerState& model;
    MixerBody body;
    juce::Viewport viewport;
    IconButton dock{ControlIcon::dock, "Show / hide send tracks", "Toggle the separate send-track dock on the right. Hiding the dock preserves all routes and send settings."};
    IconButton add{ControlIcon::add, "Create send track", "Create a mixer-only send track in the right dock. It has no arrangement lane and initially outputs to Master. Up to 32 send tracks."};
};
}

