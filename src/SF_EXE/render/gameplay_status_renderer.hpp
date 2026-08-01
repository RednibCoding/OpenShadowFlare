#ifndef OPENSHADOWFLARE_GAMEPLAY_STATUS_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_STATUS_RENDERER_HPP

namespace osf {

class GameplayStatus;
class WorldScene;

namespace gapi {
class Backend;
class NjpImage;
}

void renderGameplayStatusPanel(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& font,
    const GameplayStatus& status,
    const WorldScene& world);

}  // namespace osf

#endif
