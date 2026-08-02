#ifndef OPENSHADOWFLARE_DEBUG_PROFILING_METRICS_HPP
#define OPENSHADOWFLARE_DEBUG_PROFILING_METRICS_HPP

#include <cstdint>
#include <optional>

namespace osf::debug {

struct ProfilingMetrics {
    std::optional<std::uint64_t> ram_bytes;
    std::optional<std::uint64_t> video_memory_bytes;
    double average_framebuffer_fill_ms = 0.0;
    double average_present_ms = 0.0;
};

}  // namespace osf::debug

#endif
