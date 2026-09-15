#pragma once
#include "Theme.h"

namespace auralis
{
struct HelpContent { juce::String title, description; };
class HelpProvider
{
public:
    virtual ~HelpProvider() = default;
    virtual HelpContent helpAt(juce::Point<int>, bool keyboard = false) const = 0;
};
inline void setHelp(juce::Component& component, juce::String title, juce::String description)
{
    component.setTitle(title);
    component.setDescription(description);
}
inline HelpContent resolveHelp(juce::Component* component, juce::Point<int> screenPosition, bool keyboard = false)
{
    for (auto* current = component; current != nullptr; current = current->getParentComponent())
    {
        if (auto* provider = dynamic_cast<HelpProvider*>(current))
            return provider->helpAt(current->getLocalPoint(nullptr, screenPosition), keyboard);
        if (current->getDescription().isNotEmpty())
            return {current->getTitle(), current->getDescription()};
        if (auto* tooltip = dynamic_cast<juce::TooltipClient*>(current))
            if (tooltip->getTooltip().isNotEmpty())
                return {current->getTitle().isNotEmpty() ? current->getTitle() : (current->getName().isNotEmpty() ? current->getName() : "Library item"), tooltip->getTooltip()};
    }
    return {"Auralis workspace", "Hover a control or use Tab to focus it. Audio and MIDI sources share channel effects, sends and Master. Drop devices below; F6 enables keyboard audition. Ctrl+S saves a self-contained .aup project; Ctrl+Shift+R exports Master audio. Recording remains unavailable."};
}
class InfoView final : public juce::Component
{
public:
    InfoView()
    {
        body.setMultiLine(true);
        body.setReadOnly(true);
        body.setScrollbarsShown(true);
        body.setCaretVisible(false);
        body.setWantsKeyboardFocus(false);
        body.setColour(juce::TextEditor::backgroundColourId, colour(design::colour::panel));
        body.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        body.setFont(juce::Font(juce::FontOptions(design::fontFamily, 13.0f, juce::Font::plain)));
        body.setTitle("Contextual information");
        addAndMakeVisible(body);
        show({"Welcome to Auralis", "Hover any control or focus it with Tab to learn what it does. Add a library folder to preview samples, then drag audio to an Audio lane or the empty drop zone to edit it."});
    }
    void show(const HelpContent& content)
    {
        if (title == content.title && body.getText() == content.description) return;
        title = content.title;
        body.setText(content.description, false);
        body.moveCaretToTop(false);
        repaint();
    }
    void resized() override { body.setBounds(12, 48, getWidth()-24, getHeight()-54); }
    void paint(juce::Graphics& g) override
    {
        g.setColour(colour(design::colour::line));
        g.drawHorizontalLine(0, 12.0f, static_cast<float>(getWidth()-12));
        text(g, "INFO VIEW", {16, 5, getWidth()-32, 18}, 9, design::colour::muted, true);
        text(g, title, {16, 24, getWidth()-32, 24}, 12, design::colour::mint, true);
    }
private:
    juce::String title;
    juce::TextEditor body;
};
}



