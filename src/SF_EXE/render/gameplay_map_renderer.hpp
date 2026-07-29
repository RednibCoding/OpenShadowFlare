#ifndef OPENSHADOWFLARE_GAMEPLAY_MAP_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_MAP_RENDERER_HPP

namespace osf {

class GameplayMap;
class WorldScene;

namespace gapi {
class Backend;
class NjpImage;
}  // namespace gapi

void renderGameplayMap(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& font,
    const gapi::NjpImage& map_icons,
    const GameplayMap& map,
    const WorldScene& world);

}  // namespace osf

#endif
