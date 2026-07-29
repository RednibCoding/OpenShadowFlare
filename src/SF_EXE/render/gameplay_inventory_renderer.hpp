#ifndef OPENSHADOWFLARE_GAMEPLAY_INVENTORY_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_INVENTORY_RENDERER_HPP

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
    const WorldScene& world);

}  // namespace osf

#endif
