#pragma once
#include <cstdint>

namespace auralis::design
{
inline constexpr auto appName = "Auralis";
inline constexpr auto appVersion = "0.2.0";
inline constexpr auto windowTitle = "Auralis | Untitled session";
inline constexpr auto fontFamily = "Segoe UI";
inline constexpr int initialWidth = 1440, initialHeight = 900;
inline constexpr int minimumWidth = 1100, minimumHeight = 700;
inline constexpr int maximumWidth = 7680, maximumHeight = 4320;
inline constexpr float fontScale = 1.18f, minimumFontHeight = 11.0f;
inline constexpr int menuHeight = 32, transportHeight = 82;
inline constexpr int headerHeight = menuHeight + transportHeight, footerHeight = 8;
inline constexpr int panelGap = 8, radius = 10, padding = 16;
inline constexpr int browserWidth = 400, browserMin = 350, browserMax = 500;
namespace browser
{
inline constexpr int navigationWidth = 128, navigationMin = 116, navigationMax = 176;
inline constexpr int resultsMin = 176, headerHeight = 88, sectionHeader = 48;
inline constexpr int categoryHeight = 42, groupHeight = 32, itemHeight = 60;
inline constexpr int footerHeight = 56, indent = 14, iconSize = 18;
}
inline constexpr int devicesHeight = 210, devicesMin = 150, arrangementMin = 230;
inline constexpr int trackWidth = 228, trackMin = 200, trackMax = 320;
inline constexpr int trackHeight = 100, arrangementToolbar = 0, rulerHeight = 36;
inline constexpr int browserRow = 68, timerHz = 30, metricsIntervalMs = 1000;
inline constexpr double minTempo = 20.0, maxTempo = 300.0, defaultTempo = 120.0;
inline constexpr double minimumGainDb = -60.0, maximumGainDb = 6.0;
inline constexpr int timelineBars = 128, defaultBarWidth = 100, minBarWidth = 56, maxBarWidth = 180;
inline constexpr int previewLoopBars = 4;
inline constexpr float spectrumMinHz = 20.0f, spectrumMaxHz = 20000.0f;
namespace colour
{
inline constexpr std::uint32_t background = 0xff101419, panel = 0xff191f26, raised = 0xff222a33;
inline constexpr std::uint32_t line = 0xff303b47, text = 0xffe5edf3, muted = 0xff99a9b8;
inline constexpr std::uint32_t mint = 0xff8ee3c2, violet = 0xffb5a2f5, blue = 0xff84b7ef;
inline constexpr std::uint32_t amber = 0xffe7bd82, coral = 0xfff08d98;
inline constexpr std::uint32_t selectedLane = 0xff1b252c;
}
}

