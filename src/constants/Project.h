#pragma once
#include <cstdint>
namespace auralis::project
{
inline constexpr int formatVersion=1;
inline constexpr int maximumMetadataBytes=16*1024*1024;
inline constexpr std::int64_t maximumMediaBytes=512LL*1024*1024;
inline constexpr int maximumAutomationLanes=32,maximumAutomationPoints=1024;
inline constexpr int maximumPluginParameters=4096,maximumPluginStateBytes=16*1024*1024;
inline constexpr unsigned analysisFftSize=2048,analysisCapacity=8192,analysisBands=96;
}
