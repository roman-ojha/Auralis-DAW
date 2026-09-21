#include "model/SessionState.h"
#include <iostream>
#include <limits>
#include <stdexcept>
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
int main()
{
    try
    {
        auralis::TransportState state;
        state.advance(2); require(state.seconds == 0, "Stopped transport must not advance");
        state.playing = true; state.advance(2); require(state.barPosition() == 1, "120 BPM 4/4 bar timing");
        state.playing = false; state.advance(3); require(state.seconds == 2, "Pause preserves position");
        state.stop(); state.playing = true; state.numerator = 6; state.denominator = 8;
        state.advance(1.5); require(state.barPosition() == 1, "6/8 uses eighth-note beats");
        state.stop(); state.loop = true; state.playing = true;
        state.loopEndBeats = 12; // Explicit four-bar region in 6/8 (quarter-note units).
        state.advance(7.5); require(state.seconds == 1.5, "Loop preserves overshoot");
        state.advance(std::numeric_limits<double>::quiet_NaN()); require(state.seconds == 1.5, "Invalid timer sample ignored");
        state.setTempo(999); require(state.tempo == 300, "Tempo upper bound");
        state.setTempo(-1); require(state.tempo == 20, "Tempo lower bound");
        state.recordArmed = true; state.stop(); require(!state.recordArmed && !state.playing && state.seconds == 0, "Stop resets UI transport");
        for (int width : {1100, 1440, 1920}) for (int height : {700, 900, 1080}) for (int drag : {-10000, 10000})
        {
            auralis::PanelLayout layout; layout.browser = drag; layout.devices = drag; layout.constrain(width, height);
            require(layout.browser >= auralis::design::browserMin && layout.browser <= auralis::design::browserMax, "Browser drag remains bounded");
            require(width-layout.browser >= 650, "Two-column browser preserves arrangement space");
            require(layout.devices >= 150, "Device area remains usable");
            require(height - auralis::design::headerHeight - auralis::design::footerHeight
                - auralis::design::panelGap*3 - layout.devices >= auralis::design::arrangementMin, "Arrangement remains usable");
        }
        std::cout << "All transport and layout checks passed.\n"; return 0;
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
