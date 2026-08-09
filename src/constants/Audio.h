#pragma once
#include <cstddef>
namespace auralis::audio
{
inline constexpr std::size_t maximumSampleBytes=128u*1024u*1024u;
inline constexpr int maximumVoices=128, maximumLibraries=32;
inline constexpr double minimumRegion=0.001, previewGain=0.35;
inline constexpr int peaks=2048;
}
