#ifndef OPENSHADOWFLARE_PLAYER_LEVEL_UP_NOTICE_RENDERER_HPP
#define OPENSHADOWFLARE_PLAYER_LEVEL_UP_NOTICE_RENDERER_HPP

namespace osf {

struct PlayerLevelUpNotice;

namespace gapi {
class Backend;
class NjpImage;
}

void renderPlayerLevelUpNotice(
    gapi::Backend& renderer,
    const PlayerLevelUpNotice& notice,
    const gapi::NjpImage& font);

}  // namespace osf

#endif
