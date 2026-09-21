#include "MixerControls.h"
namespace auralis
{
IconButton::IconButton(ControlIcon image, const juce::String& title, const juce::String& help)
    : Button(title), icon(image)
{
    setHelp(*this, title, help);
    setTooltip(help);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}
void IconButton::paintButton(juce::Graphics& g, bool hover, bool down)
{
    const auto r = getLocalBounds().toFloat().reduced(2);
    g.setColour(colour(getToggleState() ? design::colour::selectedLane : design::colour::raised).brighter(hover || down ? 0.1f : 0));
    g.fillRoundedRectangle(r, 5);
    auto ink = colour(getToggleState() ? design::colour::mint : design::colour::muted);
    if (icon == ControlIcon::power) ink = colour(solo ? design::colour::amber : (muted ? design::colour::coral : design::colour::mint));
    g.setColour(isEnabled() ? ink : ink.withAlpha(0.25f));
    const auto box = r.withSizeKeepingCentre(16, 16);
    const float x = box.getX(), y = box.getY();
    switch (icon)
    {
        case ControlIcon::arrangement:
            for (int i = 0; i < 3; ++i) g.fillRoundedRectangle(x, y+static_cast<float>(i)*6, i == 1 ? 10.0f : 16.0f, 3, 1);
            break;
        case ControlIcon::mixer:
            for (int i = 0; i < 3; ++i) { const float px = x+2+static_cast<float>(i)*6; g.drawLine(px, y, px, y+16, 1); g.fillRoundedRectangle(px-2, y+(i == 1 ? 3.0f : 10.0f), 4, 4, 1); }
            break;
        case ControlIcon::piano:
            g.drawRect(box,1.0f); for(int i=1;i<4;++i)g.drawLine(x+i*4.0f,y,x+i*4.0f,y+16,1);g.fillRect(x+3,y,2.0f,9.0f);g.fillRect(x+11,y,2.0f,9.0f);break;
        case ControlIcon::play:
        {juce::Path p;p.addTriangle(x+3,y,x+3,y+16,x+15,y+8);g.fillPath(p);break;}
        case ControlIcon::pause: g.fillRect(x+2,y,4.0f,16.0f);g.fillRect(x+10,y,4.0f,16.0f);break;
        case ControlIcon::stop: g.fillRoundedRectangle(box.reduced(1),2);break;
        case ControlIcon::record: g.setColour(colour(design::colour::coral));g.fillEllipse(box.reduced(1));break;
        case ControlIcon::power:
            g.drawEllipse(box.reduced(2), 1.7f); g.drawLine(x+8, y, x+8, y+8, 2);
            if (solo) { g.drawEllipse(r.reduced(2), 1); }
            if (muted) g.drawLine(x, y+16, x+16, y, 1.5f);
            break;
        case ControlIcon::polarity:
            g.drawEllipse(box.reduced(2), 1.6f); g.drawLine(x+2, y+16, x+14, y, 1.8f); break;
        case ControlIcon::route:
        { juce::Path p; p.addTriangle(x+2,y+10,x+8,y+3,x+14,y+10); g.fillPath(p); g.drawLine(x+8,y+8,x+8,y+17,1.6f); break; }
        case ControlIcon::dock:
            g.drawRoundedRectangle(box, 2, 1.4f); g.drawLine(x+10,y,x+10,y+16,1.4f); break;
        case ControlIcon::add:
            g.drawLine(x+8,y+2,x+8,y+14,1.8f); g.drawLine(x+2,y+8,x+14,y+8,1.8f); break;
    }
    if (hasKeyboardFocus(true)) { g.setColour(colour(design::colour::mint)); g.drawRoundedRectangle(r, 5, 1); }
}
MuteSoloButton::MuteSoloButton(MixerState& model, TrackId track)
    : IconButton(ControlIcon::power, "Track mute / solo", "Click to mute/unmute. Ctrl + right-click toggles solo independently; S while focused is the keyboard equivalent. Amber ring means solo, slash means muted. UI state only."), state(model), id(track)
{
    onClick = [this] { state.toggleMute(id); };
    refresh();
}
void MuteSoloButton::refresh()
{
    if (const auto* c = state.find(id))
    {
        solo = c->solo; muted = c->muted;
        setTitle(juce::String(c->name) + " / " + (muted ? "muted" : "unmuted") + (solo ? " / solo" : ""));
        repaint();
    }
}
void MuteSoloButton::mouseDown(const juce::MouseEvent& e)
{
    soloGesture = e.mods.isRightButtonDown();
    if (soloGesture) { if (e.mods.isCtrlDown()) state.toggleSolo(id); grabKeyboardFocus(); return; }
    IconButton::mouseDown(e);
    grabKeyboardFocus();
}
void MuteSoloButton::mouseUp(const juce::MouseEvent& e)
{
    if (soloGesture) { soloGesture = false; return; }
    IconButton::mouseUp(e);
}
bool MuteSoloButton::keyPressed(const juce::KeyPress& key)
{
    if (!key.getModifiers().isAnyModifierKeyDown()
        && (key.getKeyCode() == 's' || key.getKeyCode() == 'S'))
    { state.toggleSolo(id); return true; }
    return IconButton::keyPressed(key);
}
Knob::Knob(const juce::String& title, const juce::String& help, double minimum, double maximum, double initial, std::uint32_t tint)
{
    setSliderStyle(juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle(juce::Slider::TextBoxBelow, false, 78, 18);
    setRange(minimum, maximum, 0.1);
    setValue(initial, juce::dontSendNotification);
    setDoubleClickReturnValue(true, initial);
    setNumDecimalPlacesToDisplay(1);
    setColour(juce::Slider::rotarySliderFillColourId, colour(tint));
    setHelp(*this, title, help);
    setTooltip(help);
}
void drawSilentMeter(juce::Graphics& g, juce::Rectangle<int> area, std::uint32_t, bool scale)
{
    if (scale)
    {
        for (int db = 6; db >= -60; db -= 6)
        {
            const int y = area.getY()+(6-db)*(area.getHeight()-12)/66;
            text(g, juce::String(db), {area.getX(), y-5, 23, 14}, 9, design::colour::muted, false, juce::Justification::centredRight);
        }
        area.removeFromLeft(28);
    }
    g.setColour(colour(design::colour::background)); g.fillRoundedRectangle(area.toFloat(), 3);
    g.setColour(colour(design::colour::line));
    for (int y = area.getY()+3; y < area.getBottom()-3; y += 6)
    {
        g.fillRect(area.getX()+2, y, juce::jmax(1, area.getWidth()/2-3), 2);
        g.fillRect(area.getCentreX()+1, y, juce::jmax(1, area.getWidth()/2-3), 2);
    }
}
}

