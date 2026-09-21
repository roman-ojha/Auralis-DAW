#pragma once
#include "constants/Design.h"
#include <algorithm>
#include <cmath>

namespace auralis
{
// UI preview state only. Never connected to an audio callback or recording device.
struct TransportState
{
    double tempo = design::defaultTempo;
    double seconds = 0.0;
    double loopStartBeats = 0.0, loopEndBeats = 16.0;
    int numerator = 4, denominator = 4;
    bool playing = false, recordArmed = false, metronome = false, loop = false;

    void setTempo(double value) { if (std::isfinite(value)) tempo = std::clamp(value, design::minTempo, design::maxTempo); }
    void stop() { playing = false; recordArmed = false; seconds = 0.0; }
    double barDuration() const { return 60.0 / tempo * numerator * 4.0 / denominator; }
    double barPosition() const { return seconds / barDuration(); }
    void advance(double delta)
    {
        if (!playing || !std::isfinite(delta) || delta <= 0.0) return;
        seconds += delta;
        const double loopStart = loopStartBeats*60.0/tempo;
        const double length = loop ? loopEndBeats*60.0/tempo : barDuration()*design::timelineBars;
        if (seconds >= length)
        {
            if (loop) seconds = loopStart + std::fmod(seconds-loopStart, std::max(0.001,length-loopStart));
            else { seconds = length; playing = false; }
        }
    }
};

struct PanelLayout
{
    int browser = design::browserWidth, devices = design::devicesHeight;
    void constrain(int width, int height)
    {
        browser = std::clamp(browser, design::browserMin, std::min(design::browserMax, width - 650));
        const int available = height - design::headerHeight - design::footerHeight - design::panelGap * 3;
        devices = std::clamp(devices, design::devicesMin, std::max(design::devicesMin, available - design::arrangementMin));
    }
};
}
