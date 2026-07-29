#ifndef OPENSHADOWFLARE_GAMEPLAY_HELP_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_HELP_RENDERER_HPP

#include <cstdint>

namespace osf {

class WorldScene;

namespace gapi {
class Backend;
class NjpImage;
}

void renderGameplayHelp(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& font,
    const WorldScene& world,
    std::int32_t animation_counter,
    bool close_visible,
    std::int32_t close_animation_counter);

}  // namespace osf

#endif
