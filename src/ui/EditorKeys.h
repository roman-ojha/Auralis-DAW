#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace auralis
{
struct EditorKey
{
    int code;
    bool control;
    explicit EditorKey(const juce::KeyPress& key) : code(key.getKeyCode()), control(key.getModifiers().isCtrlDown())
    {
        // Some Windows text-injection/remote-input paths deliver a control character
        // without a virtual-key scan code or held modifier. Preserve Enter/Tab/Backspace.
        if(code>=1&&code<=26&&code!=juce::KeyPress::returnKey&&code!=juce::KeyPress::tabKey&&code!=juce::KeyPress::backspaceKey)
        {
            code='A'+code-1;
            control=true;
        }
        if(code>='a'&&code<='z')code=code-'a'+'A';
    }
};
}
