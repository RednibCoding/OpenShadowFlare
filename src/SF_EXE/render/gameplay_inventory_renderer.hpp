#ifndef OPENSHADOWFLARE_GAMEPLAY_INVENTORY_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_INVENTORY_RENDERER_HPP

#include <cstdint>

namespace osf {

class GameplayInventory;
class WorldScene;

namespace gapi {
class Backend;
class NjpImage;
}  // namespace gapi

void renderGameplayInventory(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& font,
    const GameplayInventory& inventory,
    const WorldScene& world,
    std::uint32_t gameplay_counter);
void renderHeldInventoryItem(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const GameplayInventory& inventory,
    const WorldScene& world,
    std::uint32_t gameplay_counter);

}  // namespace osf

#endif
