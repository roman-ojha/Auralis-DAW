#include "WaveformPreview.h"
#include <cmath>

namespace auralis
{
WaveformPreview::WaveformPreview()
{
    configureButton(browse, "Choose audio...", "Select a local WAV, AIFF or FLAC file for a read-only waveform. No playback or track import.");
    browse.onClick = [this] { chooseFile(); };
    addAndMakeVisible(browse);
    setHelp(*this, "Audio waveform preview", "Choose a local WAV, AIFF or FLAC file to inspect its waveform. Channels are combined as absolute peaks. Maximum duration: 10 minutes. This does not play or import the file.");
    clear();
    startTimer(preview::pollMilliseconds);
}
WaveformPreview::~WaveformPreview()
{
    stopTimer();
    ++exchange->generation;
    workers.removeAllJobs(true, -1);
}
void WaveformPreview::clear()
{
    ++exchange->generation;
    { const std::lock_guard lock(exchange->mutex); exchange->ready.reset(); }
    displayed = {};
    displayed.name = "WAVEFORM";
    displayed.message = "No audio selected";
    repaint();
}
void WaveformPreview::chooseFile()
{
    if (chooser) return;
    chooser = std::make_unique<juce::FileChooser>("Select audio for waveform preview", juce::File{}, preview::filePattern);
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safe = juce::Component::SafePointer<WaveformPreview>(this)](const juce::FileChooser& dialog)
        {
            if (!safe) return;
            const auto file = dialog.getResult();
            if (file != juce::File{}) safe->load(file);
            safe->chooser.reset();
        });
}
void WaveformPreview::load(const juce::File& file)
{
    clear();
    const unsigned generation = exchange->generation.load();
    displayed.name = file.getFileName();
    displayed.message = "Reading waveform...";
    workers.addJob([shared = exchange, generation, file]
    {
        if (shared->generation.load() != generation) return;
        auto result = std::make_unique<Result>();
        result->name = file.getFileName();
        result->message = "Cannot read this audio file";
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
        if (reader && reader->sampleRate > 0 && reader->sampleRate <= preview::maximumSampleRate
            && reader->lengthInSamples > 0 && reader->numChannels > 0
            && reader->numChannels <= preview::maximumChannels)
        {
            const double seconds = static_cast<double>(reader->lengthInSamples)/reader->sampleRate;
            if (seconds > preview::maximumSeconds)
                result->message = "Preview limit: 10 minutes";
            else
            {
                juce::AudioBuffer<float> block(static_cast<int>(reader->numChannels), preview::readBlockSamples);
                bool good = true;
                for (juce::int64 position = 0; position < reader->lengthInSamples; position += preview::readBlockSamples)
                {
                    if (shared->generation.load() != generation) return;
                    const int count = static_cast<int>(juce::jmin<juce::int64>(preview::readBlockSamples, reader->lengthInSamples-position));
                    if (!reader->read(block.getArrayOfWritePointers(), block.getNumChannels(), position, count)) { good = false; break; }
                    for (int sample = 0; sample < count; ++sample)
                    {
                        const auto bin = static_cast<size_t>(static_cast<double>(position+sample)*preview::peakBins/static_cast<double>(reader->lengthInSamples));
                        for (int channel = 0; channel < block.getNumChannels(); ++channel)
                        {
                            const float value = std::abs(block.getSample(channel, sample));
                            if (std::isfinite(value)) result->peaks[juce::jmin(bin, result->peaks.size()-1)] = juce::jmax(result->peaks[juce::jmin(bin, result->peaks.size()-1)], value);
                        }
                    }
                }
                result->valid = good;
                if (good) result->message = juce::String(seconds, 2)+" s  /  "+juce::String(reader->numChannels)+" ch";
            }
        }
        const std::lock_guard lock(shared->mutex);
        if (shared->generation.load() == generation) shared->ready = std::move(result);
    });
}
void WaveformPreview::timerCallback()
{
    std::unique_ptr<Result> result;
    { const std::lock_guard lock(exchange->mutex); result = std::move(exchange->ready); }
    if (result) { displayed = std::move(*result); repaint(); }
}
void WaveformPreview::resized() { browse.setBounds(0, getHeight()-27, getWidth(), 25); }
void WaveformPreview::paint(juce::Graphics& g)
{
    text(g, displayed.name, {0, 0, getWidth(), 20}, 10, design::colour::mint, true);
    auto plot = getLocalBounds().withTrimmedTop(25).withTrimmedBottom(50);
    g.setColour(colour(design::colour::background));
    g.fillRoundedRectangle(plot.toFloat(), 4);
    if (displayed.valid && plot.getWidth() > 0)
    {
        g.setColour(colour(design::colour::mint));
        for (int x = 0; x < plot.getWidth(); ++x)
        {
            float peak = 0;
            const int first = x*preview::peakBins/plot.getWidth();
            const int end = juce::jmin(preview::peakBins, juce::jmax(first+1, (x+1)*preview::peakBins/plot.getWidth()));
            for (int bin = first; bin < end; ++bin) peak = juce::jmax(peak, displayed.peaks[static_cast<size_t>(bin)]);
            const float amplitude = juce::jmin(1.0f, peak)*static_cast<float>(plot.getHeight())/2;
            g.drawVerticalLine(plot.getX()+x, static_cast<float>(plot.getCentreY())-amplitude, static_cast<float>(plot.getCentreY())+juce::jmax(0.5f, amplitude));
        }
    }
    text(g, displayed.message, {0, getHeight()-49, getWidth(), 20}, 10, design::colour::muted);
}
}
