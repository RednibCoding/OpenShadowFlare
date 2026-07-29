#ifndef OPENSHADOWFLARE_GAMEPLAY_OPTIONS_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_OPTIONS_RENDERER_HPP

namespace osf {

class GameplayOptionsMenu;
struct GameConfig;

namespace gapi {
class Backend;
class NjpImage;
}

void renderGameplayOptions(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& font,
    const GameplayOptionsMenu& menu,
    const GameConfig& config);

}  // namespace osf

#endif
