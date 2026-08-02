#include "debug/frame_profiler.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
    }
    return condition;
}

bool near(double actual, double expected) {
    return std::abs(actual - expected) < 0.000001;
}

}  // namespace

int main() {
    osf::debug::FrameProfiler profiler;
    profiler.recordFramebufferFill(0.5);
    profiler.recordPresent(0.5);
    if (!check(
            !profiler.enabled() &&
                !profiler.memorySampleDue(0.0) &&
                near(
                    profiler.metrics().average_framebuffer_fill_ms,
                    0.0),
            "Disabled profiling retained samples or requested memory.")) {
        return 1;
    }

    profiler.setEnabled(true, 10.0);
    if (!check(
            profiler.memorySampleDue(10.0),
            "Enabling profiling did not request an immediate memory "
            "sample.")) {
        return 1;
    }
    profiler.recordMemoryUsage(
        10.0,
        std::uint64_t{32} * 1024 * 1024,
        std::uint64_t{4} * 1024 * 1024);
    if (!check(
            !profiler.memorySampleDue(10.499) &&
                profiler.memorySampleDue(10.5),
            "Memory probes are not limited to the declared interval.")) {
        return 1;
    }

    for (std::size_t sample = 1;
         sample <= osf::debug::FrameProfiler::sample_capacity;
         ++sample) {
        profiler.recordFramebufferFill(
            static_cast<double>(sample) / 1000.0);
    }
    profiler.recordPresent(0.004);
    auto metrics = profiler.metrics();
    if (!check(
            metrics.ram_bytes ==
                    std::uint64_t{32} * 1024 * 1024 &&
                metrics.video_memory_bytes ==
                    std::uint64_t{4} * 1024 * 1024 &&
                near(metrics.average_framebuffer_fill_ms, 60.5) &&
                near(metrics.average_present_ms, 4.0),
            "Profiler metrics did not preserve units or rolling means.")) {
        return 1;
    }

    profiler.recordFramebufferFill(0.121);
    metrics = profiler.metrics();
    if (!check(
            near(metrics.average_framebuffer_fill_ms, 61.5),
            "The fixed rolling window did not evict its oldest sample.")) {
        return 1;
    }

    profiler.setEnabled(false, 12.0);
    metrics = profiler.metrics();
    return check(
        !metrics.ram_bytes &&
            !metrics.video_memory_bytes &&
            near(metrics.average_framebuffer_fill_ms, 0.0) &&
            near(metrics.average_present_ms, 0.0),
        "Disabling profiling did not clear stale measurements.")
        ? 0
        : 1;
}
