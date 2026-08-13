#pragma once
#include <cstdint>
namespace auralis::editing
{
using Tick = std::int64_t;
inline constexpr Tick ppq = 960, minimumNote = 15, maximumTime = ppq * 4096;
inline constexpr int historyLimit = 100, maximumNotes = 16384, maximumClips = 2048;
inline constexpr int keyboardWidth = 76, toolbarHeight = 72, rulerHeight = 26;
inline constexpr int toolButtonWidth = 30, toolButtonGap = 3, keyboardBlackWidth = 47;
inline constexpr double lineGridPixels = 18.0;
inline constexpr std::uint32_t whiteKey = 0xffc4cbd1, blackKey = 0xff30353a;
inline constexpr std::uint32_t lightRow = 0xff242e32, darkRow = 0xff20282c, gridLine = 0xff182124;
inline constexpr int defaultRowHeight = 18, minRowHeight = 8, maxRowHeight = 32;
inline constexpr int defaultControlHeight = 116, minControlHeight = 64;
inline constexpr double pixelsPerBeat = 80, minPixelsPerBeat = 8, maxPixelsPerBeat = 320;
inline constexpr double snapBeats[] = {0, 0.25, 1.0/24, 1.0/16, 1.0/12, 0.25, 1.0/6, 1.0/4, 1.0/3, 0.5, 1, 4};
inline constexpr const char* snapNames[] = {"None", "Line", "1/6 step", "1/4 step", "1/3 step", "Step", "1/6 beat", "1/4 beat", "1/3 beat", "1/2 beat", "Beat", "Bar"};
}
