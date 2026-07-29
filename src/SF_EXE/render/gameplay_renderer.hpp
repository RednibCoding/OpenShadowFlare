#ifndef OPENSHADOWFLARE_GAMEPLAY_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_RENDERER_HPP

#include <cstdint>

namespace osf {

class WorldScene;

namespace gapi {
class Backend;
class NjpImage;
}

void renderWorld(
    gapi::Backend& renderer,
    const WorldScene& world,
    std::int32_t shadow_opacity = 500,
    const gapi::NjpImage* font = nullptr);

}  // namespace osf

#endif
