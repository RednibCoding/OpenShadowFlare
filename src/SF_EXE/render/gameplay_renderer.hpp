#ifndef OPENSHADOWFLARE_GAMEPLAY_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_RENDERER_HPP

#include <cstdint>

namespace osf {

class WorldScene;

namespace gapi {
class Backend;
class NjpImage;
}

void renderWorldGeometry(
    gapi::Backend& renderer,
    const WorldScene& world,
    std::int32_t shadow_opacity = 500,
    double interpolation = 1.0,
    bool semi_transparent_objects = true);

void renderWorld(
    gapi::Backend& renderer,
    const WorldScene& world,
    std::int32_t shadow_opacity = 500,
    const gapi::NjpImage* font = nullptr,
    double interpolation = 1.0,
    bool semi_transparent_objects = true);

}  // namespace osf

#endif
