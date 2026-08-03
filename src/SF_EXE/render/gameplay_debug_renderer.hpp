#ifndef OPENSHADOWFLARE_GAMEPLAY_DEBUG_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_DEBUG_RENDERER_HPP

#include <cstdint>

#include "debug/profiling_metrics.hpp"

namespace osf {

class GameplayDebugMenu;

namespace gapi {
class Backend;
class NjpImage;
}

void renderGameplayDebugMenu(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& font,
    const GameplayDebugMenu& menu);
void renderGameplayDebugFps(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    std::int32_t frames_per_second);
void renderGameplayProfiling(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    const debug::ProfilingMetrics& metrics,
    bool fps_counter_visible);

}  // namespace osf

#endif
