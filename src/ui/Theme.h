#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "constants/Design.h"

namespace auralis
{
inline juce::Colour colour(std::uint32_t argb) { return juce::Colour(argb); }
void text(juce::Graphics&, const juce::String&, juce::Rectangle<int>, float size = 13.0f,
          std::uint32_t ink = design::colour::text, bool bold = false,
          juce::Justification = juce::Justification::centredLeft);
void panel(juce::Graphics&, juce::Rectangle<int>);
void configureButton(juce::TextButton&, const juce::String&, const juce::String& tooltip, bool toggle = false);

class Theme final : public juce::LookAndFeel_V4
{
public:
    Theme();
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawMenuBarBackground(juce::Graphics&, int, int, bool, juce::MenuBarComponent&) override;
    void drawMenuBarItem(juce::Graphics&, int, int, int, const juce::String&, bool, bool, bool, juce::MenuBarComponent&) override;
    juce::Font getTextButtonFont(juce::TextButton&, int) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawLinearSlider(juce::Graphics&, int, int, int, int, float, float, float,
                          juce::Slider::SliderStyle, juce::Slider&) override;
};

class Splitter final : public juce::Component
{
public:
    explicit Splitter(bool vertical);
    std::function<void(int)> onDrag;
    std::function<void()> onReset;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override { repaint(); }
private:
    bool vertical;
    int lastScreenPosition = 0;
};
}
