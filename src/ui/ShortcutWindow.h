#pragma once
#include "Theme.h"
#include "constants/Shortcuts.h"
namespace auralis
{
class ShortcutWindow final : public juce::DocumentWindow
{
public:
    ShortcutWindow() : DocumentWindow("Auralis / Shortcut manual",colour(design::colour::background),DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        auto editor=std::make_unique<juce::TextEditor>();
        editor->setMultiLine(true);editor->setReadOnly(true);editor->setScrollbarsShown(true);
        editor->setText(editing::shortcutManual);editor->setFont(juce::Font(juce::FontOptions(15.0f)));
        editor->setColour(juce::TextEditor::backgroundColourId,colour(design::colour::background));
        editor->setColour(juce::TextEditor::textColourId,colour(design::colour::text));
        editor->setTitle("Auralis keyboard and mouse shortcuts");
        setContentOwned(editor.release(),true);setResizable(true,false);setResizeLimits(480,360,1600,1200);centreWithSize(740,680);
    }
    void closeButtonPressed() override { setVisible(false); }
};
}
