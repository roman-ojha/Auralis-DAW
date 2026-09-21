#include "ProcessMetrics.h"
#include <windows.h>
#include <psapi.h>
#include <algorithm>
namespace auralis
{
namespace
{
std::uint64_t ticks(FILETIME t) { return (static_cast<std::uint64_t>(t.dwHighDateTime) << 32) | t.dwLowDateTime; }
}
void ProcessMetrics::sample()
{
    FILETIME created{}, exited{}, kernel{}, user{}, wall{};
    GetSystemTimeAsFileTime(&wall);
    if (GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user))
    {
        const auto cpu = ticks(kernel) + ticks(user), now = ticks(wall);
        if (previousWall != 0 && now > previousWall)
        {
            SYSTEM_INFO info{}; GetSystemInfo(&info);
            cpuPercent = std::clamp(100.0 * static_cast<double>(cpu-previousCpu)
                / static_cast<double>(now-previousWall) / std::max(1UL, info.dwNumberOfProcessors), 0.0, 100.0);
        }
        previousCpu = cpu; previousWall = now;
    }
    PROCESS_MEMORY_COUNTERS memory{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &memory, sizeof(memory)))
        memoryMiB = static_cast<double>(memory.WorkingSetSize) / (1024.0 * 1024.0);
}
}
