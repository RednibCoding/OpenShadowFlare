#ifndef OPENSHADOWFLARE_GAMEPLAY_MAGIC_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_MAGIC_RENDERER_HPP

namespace osf {

class GameplayMagic;
class WorldScene;

namespace gapi {
class Backend;
class NjpImage;
}

void renderGameplayMagicPanel(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& magic_icons,
    const gapi::NjpImage& font,
    const GameplayMagic& panel,
    const WorldScene& world);

void renderGameplayMagicBar(
    gapi::Backend& renderer,
    const gapi::NjpImage& magic_icons,
    const gapi::NjpImage& magic_bar_icons,
    bool left_panel_active,
    bool right_panel_active,
    const WorldScene& world);

void renderHeldMagic(
    gapi::Backend& renderer,
    const gapi::NjpImage& magic_icons,
    const GameplayMagic& panel);

}  // namespace osf

#endif
