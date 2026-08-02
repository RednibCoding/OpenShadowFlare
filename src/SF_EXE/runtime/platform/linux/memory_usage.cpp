#include "runtime/platform/memory_usage.hpp"

#include <cstdio>
#include <cstdint>
#include <limits>

#include <unistd.h>

namespace osf::runtime {

std::optional<std::uint64_t> residentMemoryUsageBytes() {
    std::FILE* statm = std::fopen("/proc/self/statm", "r");
    if (!statm) {
        return std::nullopt;
    }

    unsigned long total_pages = 0;
    unsigned long resident_pages = 0;
    const int fields = std::fscanf(
        statm, "%lu %lu", &total_pages, &resident_pages);
    std::fclose(statm);
    (void) total_pages;
    const long page_size = sysconf(_SC_PAGESIZE);
    if (fields != 2 || page_size <= 0) {
        return std::nullopt;
    }
    const auto pages = static_cast<std::uint64_t>(resident_pages);
    const auto bytes_per_page = static_cast<std::uint64_t>(page_size);
    if (pages >
        std::numeric_limits<std::uint64_t>::max() /
            bytes_per_page) {
        return std::nullopt;
    }
    return pages * bytes_per_page;
}

}  // namespace osf::runtime
