#ifndef OPENSHADOWFLARE_GAMEPLAY_VENDOR_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_VENDOR_RENDERER_HPP

#include <cstdint>

namespace osf {

class GameplayVendor;
class WorldScene;

namespace gapi {
class Backend;
class NjpImage;
}  // namespace gapi

void renderGameplayVendor(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const GameplayVendor& vendor,
    const WorldScene& world,
    std::uint32_t gameplay_counter);

}  // namespace osf

#endif
