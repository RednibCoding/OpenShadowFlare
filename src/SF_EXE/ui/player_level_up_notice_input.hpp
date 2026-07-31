#ifndef OPENSHADOWFLARE_PLAYER_LEVEL_UP_NOTICE_INPUT_HPP
#define OPENSHADOWFLARE_PLAYER_LEVEL_UP_NOTICE_INPUT_HPP

#include <cstdint>

namespace osf {

class WorldScene;
struct PlayerLevelUpNotice;

namespace gapi {
class NjpImage;
}

bool playerLevelUpNoticeAcceptsPointer(
    const PlayerLevelUpNotice& notice,
    std::int32_t pointer_x,
    std::int32_t pointer_y,
    const gapi::NjpImage* font);

bool dismissPlayerLevelUpNoticeAtPointer(
    bool pointer_pressed,
    std::int32_t pointer_x,
    std::int32_t pointer_y,
    const gapi::NjpImage* font,
    WorldScene& world);

}  // namespace osf

#endif
