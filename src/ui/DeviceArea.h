#pragma once
#include "MixerControls.h"
#include "audio/DeviceProcessor.h"
namespace auralis
{
class DeviceCard;
class DeviceArea final : public juce::Component,public juce::DragAndDropTarget
{
public:
    explicit DeviceArea(MixerState&);
    ~DeviceArea() override;
    void refresh();
    void invalidate(){signature.clear();}
    std::function<SignalAnalysis::Snapshot(int)> analysis;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool isInterestedInDragSource(const SourceDetails&) override;
    void itemDropped(const SourceDetails&) override;
    std::function<void(const juce::String&,int)> onDrop;
    std::function<void(bool)> onKeyboard;
    std::function<void(int)> onEditor;
    bool keyboardEnabled() const {return keyboard.getToggleState();}
    void toggleKeyboard(){keyboard.setToggleState(!keyboard.getToggleState(),juce::sendNotification);}
private:
    MixerState& model;
    juce::Viewport viewport;
    juce::Component content;
    juce::TextButton add,keyboard;
    std::vector<std::unique_ptr<DeviceCard>> cards;
    std::string signature;
    int chainWidth=0;
};
}
