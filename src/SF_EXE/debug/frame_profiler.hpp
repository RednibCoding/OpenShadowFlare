#ifndef OPENSHADOWFLARE_DEBUG_FRAME_PROFILER_HPP
#define OPENSHADOWFLARE_DEBUG_FRAME_PROFILER_HPP

#include "debug/profiling_metrics.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace osf::debug {

class FrameProfiler {
public:
    static constexpr std::size_t sample_capacity = 120;
    static constexpr double memory_sample_interval_seconds = 0.5;

    void setEnabled(bool enabled, double now_seconds);
    bool enabled() const;

    void recordFramebufferFill(double elapsed_seconds);
    void recordPresent(double elapsed_seconds);

    bool memorySampleDue(double now_seconds) const;
    void recordMemoryUsage(
        double now_seconds,
        std::optional<std::uint64_t> game_memory_bytes,
        std::optional<std::uint64_t> audio_memory_bytes,
        std::optional<std::uint64_t> video_memory_bytes);

    ProfilingMetrics metrics() const;

private:
    class RollingAverage {
    public:
        void clear();
        void add(double value);
        double value() const;

    private:
        std::array<double, sample_capacity> samples_{};
        std::size_t count_ = 0;
        std::size_t next_ = 0;
        double total_ = 0.0;
    };

    bool enabled_ = false;
    double next_memory_sample_seconds_ = 0.0;
    RollingAverage framebuffer_fill_seconds_;
    RollingAverage present_seconds_;
    std::optional<std::uint64_t> game_memory_bytes_;
    std::optional<std::uint64_t> audio_memory_bytes_;
    std::optional<std::uint64_t> video_memory_bytes_;
};

}  // namespace osf::debug

#endif
