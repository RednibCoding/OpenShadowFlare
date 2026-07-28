#ifndef OPENSHADOWFLARE_GAMEPLAY_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_RENDERER_HPP

#include <cstdint>

namespace osf {

class WorldScene;

namespace gapi {
class Backend;
class NjpImage;
}

void renderInitialLoadingScreen(
    gapi::Backend& renderer,
    const gapi::NjpImage& waiting,
    std::int32_t counter,
    bool ready_to_continue);

void renderWorld(
    gapi::Backend& renderer,
    const WorldScene& world,
    std::int32_t shadow_opacity = 500);

}  // namespace osf

#endif
