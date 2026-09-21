#pragma once
#include <cstdint>
namespace auralis
{
// Process CPU, normalized across all logical processors, and resident memory.
// These are NOT DSP load or audio-thread deadline measurements.
class ProcessMetrics
{
public:
    void sample();
    double cpuPercent = 0.0, memoryMiB = 0.0;
private:
    std::uint64_t previousCpu = 0, previousWall = 0;
};
}
