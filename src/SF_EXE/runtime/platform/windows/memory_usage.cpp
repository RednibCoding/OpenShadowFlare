#include "runtime/platform/memory_usage.hpp"

#include <windows.h>
#include <psapi.h>

namespace osf::runtime {

std::optional<std::uint64_t> residentMemoryUsageBytes() {
    PROCESS_MEMORY_COUNTERS_EX counters{};
    if (!GetProcessMemoryInfo(
            GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
            sizeof(counters))) {
        return std::nullopt;
    }
    return static_cast<std::uint64_t>(counters.WorkingSetSize);
}

}  // namespace osf::runtime
