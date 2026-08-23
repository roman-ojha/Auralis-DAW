#pragma once
#include "Theme.h"
#include "ContextHelp.h"
#include "model/MidiProject.h"
namespace auralis
{
class AudioEditor final : public juce::Component,public HelpProvider
{
public:
    explicit AudioEditor(MidiProject&);
    void refresh();
    double progress=-1;
    void resized() override;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    HelpContent helpAt(juce::Point<int>,bool=false) const override;
    std::function<void(const AudioClip&)> onPreview;
    std::function<void()> onStop;
private:
    MidiProject& model;
    juce::TextButton play,stop,loop,reverse,mute;
    juce::Slider gain,pitch,start,end,fadeIn,fadeOut;
    juce::Rectangle<int> plot;
    bool gesture=false,refreshing=false;
    int trim=0;
    void update(int,double);
    double timeAt(int) const;
};
}
