#pragma once
#include "ContextHelp.h"
#include "constants/Preview.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>
#include <atomic>
#include <mutex>
#include "model/AudioClip.h"

namespace auralis
{
// Read-only media inspection. No audio device, transport, or arrangement connection.
class WaveformPreview final : public juce::Component, private juce::Timer
{
public:
    WaveformPreview();
    ~WaveformPreview() override;
    void chooseFile();
    void clear();
    void load(const juce::File&);
    void setProgress(double value) { if(progress!=value){progress=value;repaint();} }
    std::function<void(std::shared_ptr<const AudioData>)> onLoaded;
    std::function<void(const juce::String&)> onError;
    std::function<void()> onReplay,onClear;
    void mouseDown(const juce::MouseEvent&) override { if(displayed.valid&&onReplay)onReplay(); }
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    struct Result
    {
        std::array<float, preview::peakBins> peaks{};
        juce::String name, message;
        bool valid = false;
        std::shared_ptr<AudioData> audio;
    };
    struct Exchange
    {
        std::atomic<unsigned> generation{0};
        std::mutex mutex;
        std::unique_ptr<Result> ready;
    };
    std::shared_ptr<Exchange> exchange = std::make_shared<Exchange>();
    juce::ThreadPool workers{1};
    std::unique_ptr<juce::FileChooser> chooser;
    Result displayed;
    double progress=-1;
    void timerCallback() override;
};
}
