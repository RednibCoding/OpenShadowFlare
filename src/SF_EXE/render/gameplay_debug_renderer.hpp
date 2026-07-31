#ifndef OPENSHADOWFLARE_GAMEPLAY_DEBUG_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_DEBUG_RENDERER_HPP

#include <cstdint>

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

}  // namespace osf

#endif
