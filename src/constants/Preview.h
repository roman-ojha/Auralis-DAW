#pragma once
namespace auralis::preview
{
inline constexpr int infoHeight = 136, waveformHeight = 126;
inline constexpr int peakBins = 2048, readBlockSamples = 8192, pollMilliseconds = 100;
inline constexpr double maximumSeconds = 600.0;
inline constexpr double maximumSampleRate = 384000.0;
inline constexpr int maximumChannels = 32;
inline constexpr auto filePattern = "*.wav;*.aiff;*.aif;*.flac";
}
