#include "Transport.h"
#include <array>
#include <cmath>
namespace auralis
{
Transport::Transport(TransportState& s) : state(s)
{
    addAndMakeVisible(arrangementView); addAndMakeVisible(mixerView); addAndMakeVisible(pianoView);
    arrangementView.setToggleState(true, juce::dontSendNotification);
    arrangementView.onClick = [this] { showView(0); };
    mixerView.onClick = [this] { showView(1); };
    pianoView.onClick = [this] { showView(2); };
    record.setClickingTogglesState(true);
    configureButton(metro, "Click", "Ctrl+M: metronome toggle - UI only, no sound", true);
    configureButton(loop, "Loop", "L: toggle selected transport loop. Drag arrangement ruler or Ctrl+L on a selected time range to set its boundaries.", true);

    for (auto* b : std::initializer_list<juce::Component*>{ &play, &pause, &stop, &record, &metro, &loop }) addAndMakeVisible(b);
    play.onClick = [this] { state.playing = true; refresh(); };
    pause.onClick = [this] { state.playing = false; refresh(); };
    stop.onClick = [this] { state.stop(); refresh(); };
    record.onClick = [this] { state.recordArmed = record.getToggleState(); refresh(); };
    metro.onClick = [this] { state.metronome = metro.getToggleState(); };
    loop.onClick = [this] { state.loop = loop.getToggleState(); };

    tempo.setSliderStyle(juce::Slider::IncDecButtons);
    tempo.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 56, 28);
    tempo.setRange(design::minTempo, design::maxTempo, 0.1);
    tempo.setValue(state.tempo); tempo.setNumDecimalPlacesToDisplay(1);
    tempo.setTitle("Tempo in beats per minute"); tempo.setTooltip("Tempo: 20 to 300 BPM; UI preview only");
    tempo.onValueChange = [this] { state.setTempo(tempo.getValue()); };
    addAndMakeVisible(tempo);
    signature.addItemList({"4/4", "3/4", "5/4", "6/8", "7/8", "12/8"}, 1);
    signature.setSelectedId(1); signature.setTitle("Time signature");
    signature.setDescription("Choose beats per bar and beat unit for the UI timeline. This affects the bar counter and preview loop duration; there is no audio timing engine yet.");
    signature.onChange = [this]
    {
        const auto parts = juce::StringArray::fromTokens(signature.getText(), "/", "");
        if (parts.size() == 2) { state.numerator = parts[0].getIntValue(); state.denominator = parts[1].getIntValue(); }
    };
    for(int i=0;i<12;++i)quantize.addItem(editing::snapNames[i],i+1);
    quantize.setSelectedId(8); quantize.setTitle("Quantization preference");
    quantize.setTooltip("Shared arrangement/piano-roll snap grid. Backspace toggles Line/None. Alt-drag bypasses snap; triplet and step options are available.");
    quantize.onChange=[this]{if(onSnapChanged)onSnapChanged(quantize.getSelectedId()-1);};
    addAndMakeVisible(signature); addAndMakeVisible(quantize);
    sampleMetrics(); refresh();
}
void Transport::refresh()
{
    play.setToggleState(state.playing, juce::dontSendNotification);
    record.setToggleState(state.recordArmed, juce::dontSendNotification);
    metro.setToggleState(state.metronome, juce::dontSendNotification);
    loop.setToggleState(state.loop, juce::dontSendNotification);
    repaint();
}
void Transport::showView(int view)
{
    arrangementView.setToggleState(view==0, juce::dontSendNotification);
    mixerView.setToggleState(view==1, juce::dontSendNotification);
    pianoView.setToggleState(view==2,juce::dontSendNotification);
    if (onViewChanged) onViewChanged(view);
}
HelpContent Transport::helpAt(juce::Point<int> point, bool) const
{
    if (spectrumBounds.contains(point)) return {"Audio spectrum", "A logarithmic 20 Hz to 20 kHz frequency display. It shows no signal because no audio engine is connected."};
    if (timeBounds.contains(point)) return {"Transport position", "Elapsed minutes, seconds and milliseconds, with bar and beat below. This is a message-thread UI preview, not a sample-accurate audio clock."};
    if (point.x >= getWidth()-146-mixing::switchWidth) return {"Application resources / views", "CPU is Auralis process usage normalized across logical processors. RAM is resident process memory in MiB. The icons to the right switch between arrangement and mixer."};
    return {"Transport", "Play, pause and stop the silent timeline preview. Tempo, signature and loop affect the preview. Recording and metronome are UI toggles only."};
}
void Transport::resized()
{

    int x = 20;
    for (auto* b : { &play, &pause, &stop, &record }) { b->setBounds(x, 28, 47, 32); x += 53; }
    timeBounds = { x+10, 17, 124, 52 }; x += 140;
    tempoBounds = {x, 10, 96, 54}; tempo.setBounds(x, 31, 94, 29); x += 100;
    signatureBounds = {x, 10, 70, 54}; signature.setBounds(x, 31, 66, 29); x += 76;
    quantizeBounds = {x, 10, 78, 54}; quantize.setBounds(x, 31, 72, 29); x += 80;
    metro.setBounds(x, 28, 51, 32); x += 57;
    loop.setBounds(x, 28, 50, 32); x += 64;
    spectrumBounds = {x, 9, getWidth()-x-146-mixing::switchWidth, 62};
    pianoView.setBounds(getWidth()-mixing::switchWidth, 28, mixing::switchButtonSize, mixing::switchButtonSize);
    arrangementView.setBounds(getWidth()-mixing::switchWidth+34, 28, mixing::switchButtonSize, mixing::switchButtonSize);
    mixerView.setBounds(getWidth()-mixing::switchWidth+68, 28, mixing::switchButtonSize, mixing::switchButtonSize);
}
void Transport::drawSpectrum(juce::Graphics& g)
{
    auto r = spectrumBounds;
    g.setColour(colour(design::colour::background)); g.fillRoundedRectangle(r.toFloat(), 6);
    text(g, "SPECTRUM / NO SIGNAL", r.reduced(8).withHeight(12), 9, design::colour::muted, true);
    const auto plot = r.reduced(9, 0).withTrimmedTop(23).withTrimmedBottom(17);
    for (const float frequency : std::array<float, 4>{20, 200, 2000, 20000})
    {
        const float fraction = std::log10(frequency/design::spectrumMinHz) / std::log10(design::spectrumMaxHz/design::spectrumMinHz);
        const int x = plot.getX() + juce::roundToInt(fraction * static_cast<float>(plot.getWidth()));
        g.setColour(colour(design::colour::line)); g.drawVerticalLine(x, static_cast<float>(plot.getY()), static_cast<float>(plot.getBottom()));
        const auto label = frequency < 1000 ? juce::String(static_cast<int>(frequency)) : juce::String(static_cast<int>(frequency/1000)) + "k";
        text(g, label, {juce::jlimit(r.getX()+2, r.getRight()-28, x-12), r.getBottom()-17, 27, 13}, 9, design::colour::muted);
    }
    g.setColour(colour(design::colour::mint).withAlpha(0.5f));
    g.drawHorizontalLine(plot.getBottom()-1, static_cast<float>(plot.getX()), static_cast<float>(plot.getRight()));
}
void Transport::paint(juce::Graphics& g)
{
    g.fillAll(colour(design::colour::background));
    text(g, "TRANSPORT", {20, 7, 170, 15}, 9, design::colour::muted, true);
    const int total = static_cast<int>(state.seconds);
    const auto clock = juce::String::formatted("%02d:%02d.%03d", total/60, total%60, static_cast<int>((state.seconds-total)*1000));
    text(g, clock, timeBounds.withHeight(27), 22, design::colour::text, true);
    const int beat = static_cast<int>(std::fmod(state.barPosition(), 1.0) * state.numerator) + 1;
    text(g, juce::String(static_cast<int>(state.barPosition())+1).paddedLeft('0', 3) + " . " + juce::String(beat).paddedLeft('0', 2) + "   /   BARS", timeBounds.withTrimmedTop(28), 10, design::colour::mint);
    text(g, "BPM", tempoBounds.withHeight(16), 9, design::colour::muted, true);
    text(g, "SIGNATURE", signatureBounds.withHeight(16), 9, design::colour::muted, true);
    text(g, "SNAP", quantizeBounds.withHeight(16), 9, design::colour::muted, true);
    drawSpectrum(g);
    const int x = getWidth()-130-mixing::switchWidth;
    text(g, "APP RESOURCES", {x, 7, 115, 15}, 9, design::colour::muted, true);
    text(g, "CPU   " + juce::String(metrics.cpuPercent, 1) + "%", {x, 26, 115, 18}, 11, design::colour::mint);
    text(g, "RAM  " + juce::String(juce::roundToInt(metrics.memoryMiB)) + " MiB", {x, 47, 115, 18}, 11, design::colour::text);
}
}


