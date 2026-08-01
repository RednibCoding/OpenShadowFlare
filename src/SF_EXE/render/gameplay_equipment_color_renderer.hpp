#ifndef OPENSHADOWFLARE_GAMEPLAY_EQUIPMENT_COLOR_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_EQUIPMENT_COLOR_RENDERER_HPP

#include <cstdint>

namespace osf {

class GameplayEquipmentColor;
class WorldScene;

namespace gapi {
class Backend;
class NjpImage;
}

void renderGameplayEquipmentColor(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const GameplayEquipmentColor& equipment_color,
    const WorldScene& world,
    std::uint32_t gameplay_counter);

}  // namespace osf

#endif
