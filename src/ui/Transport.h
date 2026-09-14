#pragma once
#include "Theme.h"
#include "audio/SignalAnalysis.h"
#include "ContextHelp.h"
#include "MixerControls.h"
#include "model/SessionState.h"
#include "constants/Editing.h"
#include "platform/ProcessMetrics.h"
namespace auralis
{
class Transport final : public juce::Component, public HelpProvider
{
public:
    explicit Transport(TransportState&);
    void paint(juce::Graphics&) override;
    void resized() override;
    void refresh();
    void showView(int);
    SignalAnalysis::Snapshot signal;
    void setSnap(int index) { quantize.setSelectedId(index+1,juce::dontSendNotification); }
    std::function<void(int)> onViewChanged;
    std::function<void(int)> onSnapChanged;
    std::function<void()> onStopAudition;
    HelpContent helpAt(juce::Point<int>, bool keyboard = false) const override;
    void sampleMetrics() { metrics.sample(); }

private:
    TransportState& state;
    ProcessMetrics metrics;
    IconButton arrangementView{ControlIcon::arrangement, "Arrangement view", "F5: show the arrangement lanes. All channel controls share their state with the mixer."};
    IconButton mixerView{ControlIcon::mixer, "Mixer view", "F9: show the mixer, master/current meters and send-track dock. All channel controls share their state with the arrangement."};
    IconButton pianoView{ControlIcon::piano,"Piano roll / F7","F7: open the selected independent MIDI clip. Create clips by double-clicking an empty instrument lane."};
    IconButton play{ControlIcon::play,"Play / Space","Space starts/stops arrangement audio. Ctrl+Space starts/pauses. Loaded instruments play MIDI clips through the same mixer graph."};
    IconButton pause{ControlIcon::pause,"Pause / Ctrl+Space","Ctrl+Space: pause/resume at the current position."};
    IconButton stop{ControlIcon::stop,"Stop / Escape","Escape: stop playback and return to the beginning."};
    IconButton record{ControlIcon::record,"Record arm / R","R toggles recording arm. UI only; no recording yet."};
    juce::TextButton metro, loop;
    juce::Slider tempo;
    juce::ComboBox signature, quantize;
    juce::Rectangle<int> spectrumBounds, timeBounds, tempoBounds, signatureBounds, quantizeBounds;
    void drawSpectrum(juce::Graphics&);
};
}

