#ifndef OPENSHADOWFLARE_ITEM_INFORMATION_RENDERER_HPP
#define OPENSHADOWFLARE_ITEM_INFORMATION_RENDERER_HPP

namespace osf {

class GameplayInventory;
class WorldScene;

namespace gapi {
class Backend;
class NjpImage;
}  // namespace gapi

void renderItemInformation(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    const GameplayInventory& inventory,
    const WorldScene& world);

}  // namespace osf

#endif
