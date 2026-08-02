#include "frame_profiler.hpp"

#include <algorithm>

namespace osf::debug {

void FrameProfiler::setEnabled(
    bool enabled,
    double now_seconds) {
    if (enabled_ == enabled) {
        return;
    }
    enabled_ = enabled;
    framebuffer_fill_seconds_.clear();
    present_seconds_.clear();
    game_memory_bytes_.reset();
    audio_memory_bytes_.reset();
    video_memory_bytes_.reset();
    next_memory_sample_seconds_ = now_seconds;
}

bool FrameProfiler::enabled() const {
    return enabled_;
}

void FrameProfiler::recordFramebufferFill(
    double elapsed_seconds) {
    if (enabled_) {
        framebuffer_fill_seconds_.add(
            std::max(elapsed_seconds, 0.0));
    }
}

void FrameProfiler::recordPresent(double elapsed_seconds) {
    if (enabled_) {
        present_seconds_.add(std::max(elapsed_seconds, 0.0));
    }
}

bool FrameProfiler::memorySampleDue(double now_seconds) const {
    return enabled_ &&
           now_seconds >= next_memory_sample_seconds_;
}

void FrameProfiler::recordMemoryUsage(
    double now_seconds,
    std::optional<std::uint64_t> game_memory_bytes,
    std::optional<std::uint64_t> audio_memory_bytes,
    std::optional<std::uint64_t> video_memory_bytes) {
    if (!enabled_) {
        return;
    }
    game_memory_bytes_ = game_memory_bytes;
    audio_memory_bytes_ = audio_memory_bytes;
    video_memory_bytes_ = video_memory_bytes;
    next_memory_sample_seconds_ =
        now_seconds + memory_sample_interval_seconds;
}

ProfilingMetrics FrameProfiler::metrics() const {
    return {
        game_memory_bytes_,
        audio_memory_bytes_,
        video_memory_bytes_,
        framebuffer_fill_seconds_.value() * 1000.0,
        present_seconds_.value() * 1000.0,
    };
}

void FrameProfiler::RollingAverage::clear() {
    samples_.fill(0.0);
    count_ = 0;
    next_ = 0;
    total_ = 0.0;
}

void FrameProfiler::RollingAverage::add(double value) {
    if (count_ < samples_.size()) {
        samples_[next_] = value;
        total_ += value;
        ++count_;
    } else {
        total_ -= samples_[next_];
        samples_[next_] = value;
        total_ += value;
    }
    next_ = (next_ + 1) % samples_.size();
}

double FrameProfiler::RollingAverage::value() const {
    return count_ == 0
        ? 0.0
        : total_ / static_cast<double>(count_);
}

}  // namespace osf::debug
