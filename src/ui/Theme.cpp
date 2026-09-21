#include "Theme.h"
namespace auralis
{

void text(juce::Graphics& g, const juce::String& value, juce::Rectangle<int> bounds,
          float size, std::uint32_t ink, bool bold, juce::Justification align)
{
    g.setColour(colour(ink));
    g.setFont(juce::Font(juce::FontOptions(design::fontFamily, juce::jmax(design::minimumFontHeight, size * design::fontScale), bold ? juce::Font::bold : juce::Font::plain)));
    g.drawFittedText(value, bounds, align, 1, 1.0f);
}
void panel(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(colour(design::colour::panel));
    g.fillRoundedRectangle(bounds.toFloat(), static_cast<float>(design::radius));
}
void configureButton(juce::TextButton& b, const juce::String& title, const juce::String& tooltip, bool toggle)
{
    b.setButtonText(title); b.setTooltip(tooltip); b.setTitle(title); b.setDescription(tooltip);
    b.setClickingTogglesState(toggle); b.setMouseCursor(juce::MouseCursor::PointingHandCursor);
}
Theme::Theme()
{
    setColour(juce::TextButton::buttonOnColourId, colour(design::colour::mint));
    setColour(juce::TextButton::textColourOffId, colour(design::colour::text));
    setColour(juce::TextButton::textColourOnId, colour(design::colour::background));
    setColour(juce::ComboBox::backgroundColourId, colour(design::colour::raised));
    setColour(juce::ComboBox::textColourId, colour(design::colour::text));
    setColour(juce::ComboBox::outlineColourId, colour(design::colour::line));
    setColour(juce::PopupMenu::backgroundColourId, colour(design::colour::raised));
    setColour(juce::PopupMenu::textColourId, colour(design::colour::text));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, colour(design::colour::mint));
    setColour(juce::PopupMenu::highlightedTextColourId, colour(design::colour::background));
    setColour(juce::TextEditor::backgroundColourId, colour(design::colour::background));
    setColour(juce::TextEditor::textColourId, colour(design::colour::text));
    setColour(juce::TextEditor::outlineColourId, colour(design::colour::line));
    setColour(juce::TextEditor::focusedOutlineColourId, colour(design::colour::mint));
    setColour(juce::Slider::textBoxTextColourId, colour(design::colour::text));
    setColour(juce::Slider::textBoxBackgroundColourId, colour(design::colour::raised));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ScrollBar::thumbColourId, colour(design::colour::line));
    setColour(juce::TooltipWindow::backgroundColourId, colour(design::colour::raised));
    setColour(juce::TooltipWindow::textColourId, colour(design::colour::text));
}
juce::Font Theme::getTextButtonFont(juce::TextButton&, int) { return juce::Font(juce::FontOptions(design::fontFamily, 13.0f, juce::Font::bold)); }
void Theme::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float position, float start, float end, juce::Slider& slider)
{
    const float diameter = static_cast<float>(juce::jmin(width, height))-8;
    if (diameter <= 0) return;
    const float cx = static_cast<float>(x)+static_cast<float>(width)/2;
    const float cy = static_cast<float>(y)+static_cast<float>(height)/2;
    const float radius = diameter/2;
    const float angle = start+position*(end-start);
    juce::Path arc;
    arc.addCentredArc(cx, cy, radius, radius, 0, start, end, true);
    g.setColour(colour(design::colour::line)); g.strokePath(arc, juce::PathStrokeType(2));
    arc.clear(); arc.addCentredArc(cx, cy, radius, radius, 0, start, angle, true);
    g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId).withAlpha(slider.isEnabled() ? 1.0f : 0.25f));
    g.strokePath(arc, juce::PathStrokeType(2));
    auto body = juce::Rectangle<float>(cx-radius+4, cy-radius+4, diameter-8, diameter-8);
    g.setColour(juce::Colours::black.withAlpha(0.45f)); g.fillEllipse(body.translated(0, 2));
    g.setGradientFill(juce::ColourGradient(colour(design::colour::line).brighter(0.1f), cx, body.getY(), colour(design::colour::background), cx, body.getBottom(), false));
    g.fillEllipse(body);
    g.setColour(colour(design::colour::background)); g.drawEllipse(body, 1);
    const float reach = juce::jmax(1.0f, radius-7);
    g.setColour(slider.isEnabled() ? colour(design::colour::text) : colour(design::colour::muted));
    g.drawLine(cx+std::sin(angle)*reach*0.45f, cy-std::cos(angle)*reach*0.45f,
               cx+std::sin(angle)*reach, cy-std::cos(angle)*reach, 2);
    if (slider.hasKeyboardFocus(true)) { g.setColour(colour(design::colour::mint)); g.drawEllipse(body.expanded(2), 1); }
}
void Theme::drawMenuBarBackground(juce::Graphics& g, int width, int height, bool, juce::MenuBarComponent&)
{
    g.fillAll(colour(design::colour::background));
    g.setColour(colour(design::colour::line));
    g.drawHorizontalLine(height-1, 0, static_cast<float>(width));
}
void Theme::drawMenuBarItem(juce::Graphics& g, int width, int height, int, const juce::String& name,
                          bool hovered, bool open, bool, juce::MenuBarComponent&)
{
    if (hovered || open)
    {
        g.setColour(colour(design::colour::raised));
        g.fillRoundedRectangle(2, 3, static_cast<float>(width-4), static_cast<float>(height-6), 4);
    }
    text(g, name, {0, 0, width, height}, 11, open ? design::colour::mint : design::colour::text, false, juce::Justification::centred);
}
juce::Font Theme::getComboBoxFont(juce::ComboBox&) { return juce::Font(juce::FontOptions(design::fontFamily, 13.0f, juce::Font::plain)); }
void Theme::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hover, bool down)
{
    auto c = b.getToggleState() ? b.findColour(juce::TextButton::buttonOnColourId) : colour(design::colour::raised);
    if (hover || down) c = c.brighter(down ? 0.15f : 0.07f);
    if (!b.isEnabled()) c = c.withAlpha(0.4f);
    g.setColour(c); g.fillRoundedRectangle(b.getLocalBounds().toFloat().reduced(1.0f), 6.0f);
    if (b.hasKeyboardFocus(true)) { g.setColour(colour(design::colour::mint)); g.drawRoundedRectangle(b.getLocalBounds().toFloat().reduced(1.0f), 6.0f, 1.5f); }
}
void Theme::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float pos,
                            float, float, juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (style == juce::Slider::LinearVertical)
    {
        const float cx = static_cast<float>(x)+static_cast<float>(width)/2;
        g.setColour(colour(design::colour::background)); g.fillRoundedRectangle(cx-3, static_cast<float>(y), 6, static_cast<float>(height), 3);
        g.setColour(s.findColour(juce::Slider::thumbColourId).withAlpha(0.45f)); g.fillRect(cx-1, pos, 2.0f, juce::jmax(0.0f, static_cast<float>(y+height)-pos));
        g.setGradientFill(juce::ColourGradient(colour(design::colour::muted), cx, pos-8, colour(design::colour::raised), cx, pos+8, false));
        g.fillRoundedRectangle(cx-13, pos-8, 26, 16, 3);
        g.setColour(s.findColour(juce::Slider::thumbColourId)); g.drawLine(cx-11, pos, cx+11, pos, 2);
        return;
    }
    const float cy = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
    g.setColour(colour(design::colour::line));
    g.fillRoundedRectangle(static_cast<float>(x), cy - 2, static_cast<float>(width), 4, 2);
    g.setColour(s.findColour(juce::Slider::thumbColourId));
    g.fillRoundedRectangle(static_cast<float>(x), cy - 2, juce::jmax(0.0f, pos - static_cast<float>(x)), 4, 2);
    g.fillEllipse(pos - 5, cy - 5, 10, 10);
}
Splitter::Splitter(bool isVertical) : vertical(isVertical)
{
    setMouseCursor(vertical ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::UpDownResizeCursor);
    setTitle(vertical ? "Resize columns" : "Resize device area");
    setDescription("Drag this divider to resize the adjacent sections. Double-click to restore this divider, or use View > Reset Layout to restore all panel sizes.");
}
void Splitter::paint(juce::Graphics& g)
{
    g.setColour(colour(isMouseOverOrDragging() ? design::colour::mint : design::colour::line));
    auto r = getLocalBounds().toFloat();
    if (vertical) g.fillRoundedRectangle(r.getCentreX() - 1, r.getCentreY() - 18, 2, 36, 1);
    else g.fillRoundedRectangle(r.getCentreX() - 18, r.getCentreY() - 1, 36, 2, 1);
}
void Splitter::mouseDown(const juce::MouseEvent& e) { lastScreenPosition = vertical ? e.getScreenX() : e.getScreenY(); }
void Splitter::mouseDrag(const juce::MouseEvent& e)
{
    const int next = vertical ? e.getScreenX() : e.getScreenY();
    if (onDrag) onDrag(next - lastScreenPosition);
    lastScreenPosition = next;
}
void Splitter::mouseDoubleClick(const juce::MouseEvent&) { if (onReset) onReset(); }
}

