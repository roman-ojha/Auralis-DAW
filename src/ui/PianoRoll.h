#pragma once
#include "Theme.h"
#include "ContextHelp.h"
#include "model/MidiProject.h"
#include <set>
namespace auralis
{
class PianoRoll final : public juce::Component, public HelpProvider, private juce::ScrollBar::Listener
{
public:
    explicit PianoRoll(MidiProject&);
    void refresh();
    void resized() override;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed(const juce::KeyPress&) override;
    HelpContent helpAt(juce::Point<int>, bool = false) const override;
private:
    MidiProject& model;
    juce::ComboBox tools, snap, property, cycle;
    juce::TextButton quantize;
    juce::ScrollBar horizontal{false}, vertical{true};
    Splitter controlDivider{false};
    juce::Rectangle<int> grid, controls;
    std::set<int> selected;
    std::vector<MidiNote> originals, clipboard;
    int activeSeen = 0, rowHeight = editing::defaultRowHeight, controlHeight = editing::defaultControlHeight;
    double pixels = editing::pixelsPerBeat, offset = 0, pitchOffset = 48;
    Tick lastLength = editing::ppq, anchorTime = 0;
    int anchorPitch = 60, dragId = 0;
    juce::Point<int> anchor, cursor;
    enum class Drag { none, selection, move, resize, erase, paint, pan, control } drag = Drag::none;
    double initialOffset = 0, initialPitchOffset = 0;
    std::set<int> previousSelection;
    void scrollBarMoved(juce::ScrollBar*, double) override;
    void updateRanges();
    Tick timeAt(int x, bool bypass = false) const;
    int pitchAt(int y) const;
    juce::Rectangle<int> noteBounds(const MidiNote&) const;
    MidiNote* noteAt(juce::Point<int>);
    void eraseAt(juce::Point<int>);
    void drawAt(const juce::MouseEvent&);
    void controlAt(juce::Point<int>);
    void zoom(double factor);
    void quantizeNotes();
    bool chosen(const MidiNote& n) const { return selected.empty() || selected.count(n.id) != 0; }
};
}
