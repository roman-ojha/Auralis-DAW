#pragma once
#include "ContextHelp.h"
#include "model/SessionState.h"

namespace auralis
{
class ApplicationMenu final : public juce::Component, private juce::MenuBarModel
{
public:
    explicit ApplicationMenu(TransportState& transport) : state(transport), bar(this)
    {
        addAndMakeVisible(bar);
        setHelp(bar, "Application menus", "File: choose waveform audio or exit. Playback: control the silent timeline preview. View: reset all panel sizes. Grey commands are reserved for future implementation. Use the arrow keys inside an open menu.");
    }
    ~ApplicationMenu() override { bar.setModel(nullptr); }
    std::function<void()> onReset, onChooseAudio, onTransportChange;
    std::function<void(int)> onView;
    std::function<void()> onShortcuts, onCreateClip;
    std::function<void(bool)> onUndo;
    void resized() override { bar.setBounds(getLocalBounds()); }
private:
    enum Command { chooseAudio = 1, quit, play, pause, stop, record, metronome, loop, reset, about, arrangement, mixer, piano, shortcuts, createClip, undo, redo, future = 100 };
    TransportState& state;
    juce::MenuBarComponent bar;
    juce::StringArray getMenuBarNames() override { return {"File", "Edit", "Create", "Playback", "View", "Navigate", "Options", "Help"}; }
    juce::PopupMenu getMenuForIndex(int index, const juce::String&) override
    {
        juce::PopupMenu menu;
        auto later = [&menu](const juce::String& label) { menu.addItem(future, label + " (planned)", false); };
        switch (index)
        {
            case 0:
                later("New project"); later("Open project..."); later("Save project"); later("Export audio...");
                menu.addSeparator(); menu.addItem(chooseAudio, "Choose audio for waveform...");
                menu.addSeparator(); menu.addItem(quit, "Exit Auralis"); break;
            case 1: menu.addItem(undo,"Undo MIDI edit (Ctrl+Z)"); menu.addItem(redo,"Redo MIDI edit (Ctrl+Y)"); menu.addSeparator(); later("Cut"); later("Copy"); later("Paste"); later("Delete"); break;
            case 2: later("Audio track"); later("Instrument track"); later("Return track"); menu.addItem(createClip,"MIDI clip at cursor (Ctrl+Shift+M)"); break;
            case 3:
                menu.addSectionHeader("Silent UI preview");
                menu.addItem(play, "Play", true, state.playing); menu.addItem(pause, "Pause"); menu.addItem(stop, "Stop / return to start");
                menu.addItem(record, "Arm recording UI", true, state.recordArmed);
                menu.addItem(metronome, "Metronome UI", true, state.metronome); menu.addItem(loop, "Loop selected range (L)", true, state.loop); break;
            case 4: menu.addItem(reset, "Reset Layout"); menu.addSeparator(); menu.addItem(arrangement, "Arrangement (F5)"); menu.addItem(mixer, "Mixer (F9)"); menu.addItem(piano,"Piano roll (F7)"); later("Automation lanes"); break;
            case 5: later("Go to marker"); later("Next clip"); later("Previous clip"); break;
            case 6: later("Audio devices..."); later("MIDI devices..."); later("Plugin locations..."); later("Preferences..."); break;
            case 7: menu.addItem(about, "About Auralis / current capabilities"); menu.addItem(shortcuts,"Shortcut manual (F1)"); break;
            default: break;
        }
        return menu;
    }
    void menuItemSelected(int command, int) override
    {
        switch (command)
        {
            case chooseAudio: if (onChooseAudio) onChooseAudio(); return;
            case quit: juce::JUCEApplication::getInstance()->systemRequestedQuit(); return;
            case reset: if (onReset) onReset(); return;
            case arrangement: if (onView) onView(0); return;
            case mixer: if (onView) onView(1); return;
            case createClip: if(onCreateClip)onCreateClip();return;
            case piano: if(onView)onView(2); return;
            case shortcuts: if(onShortcuts)onShortcuts();return;
            case undo: if(onUndo)onUndo(false);return;
            case redo: if(onUndo)onUndo(true);return;
            case play: state.playing = true; break;
            case pause: state.playing = false; break;
            case stop: state.stop(); break;
            case record: state.recordArmed = !state.recordArmed; break;
            case metronome: state.metronome = !state.metronome; break;
            case loop: state.loop = !state.loop; break;
            case about:
                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, "Auralis",
                    "A native C++ creative workspace.\n\nUI prototype: silent transport, library, linked arrangement/mixer, send routing and device path. Local audio waveform inspection is read-only. No audio engine, recording, plugin hosting or project saving yet.\n\nHover controls or use Tab for Info View help. Drag dividers to resize; double-click a divider or choose View > Reset Layout to restore sizes."); return;
            default: return;
        }
        if (onTransportChange) onTransportChange();
    }
};
}

