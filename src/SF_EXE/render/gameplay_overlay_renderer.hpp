#ifndef OPENSHADOWFLARE_GAMEPLAY_OVERLAY_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_OVERLAY_RENDERER_HPP

#include <cstdint>

namespace osf {

class WorldScene;

namespace gapi {
class Backend;
class NjpImage;
}

void renderGameplayOverlay(
    gapi::Backend& renderer,
    const WorldScene& world,
    const gapi::NjpImage* font,
    std::int32_t camera_x,
    std::int32_t camera_y);

}  // namespace osf

#endif
