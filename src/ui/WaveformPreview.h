#pragma once
#include "ContextHelp.h"
#include "constants/Preview.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>
#include <atomic>
#include <mutex>

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
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    struct Result
    {
        std::array<float, preview::peakBins> peaks{};
        juce::String name, message;
        bool valid = false;
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
    juce::TextButton browse;
    Result displayed;
    void load(const juce::File&);
    void timerCallback() override;
};
}
