#ifndef OPENSHADOWFLARE_GAMEPLAY_MAP_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_MAP_RENDERER_HPP

namespace osf {

class GameplayMap;
class WorldScene;

namespace gapi {
class Backend;
class NjpImage;
struct Viewport;
}  // namespace gapi

void renderGameplayMap(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& font,
    const gapi::NjpImage& map_icons,
    const GameplayMap& map,
    const WorldScene& world);

// Renders only the explored world map and player marker. This is intended for
// small secondary displays, so it deliberately omits the map screen frame,
// title, and animated effects.
void renderGameplayMiniMap(
    gapi::Backend& renderer,
    const gapi::NjpImage& map_icons,
    const WorldScene& world,
    gapi::Viewport viewport);

}  // namespace osf

#endif
