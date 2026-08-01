#include "ui/player_level_up_notice_input.hpp"

#include "ui/player_level_up_notice_layout.hpp"
#include "world/world_scene.hpp"

namespace osf {

bool playerLevelUpNoticeAcceptsPointer(
    const PlayerLevelUpNotice& notice,
    std::int32_t pointer_x,
    std::int32_t pointer_y,
    const gapi::NjpImage* font) {
    if (!font || !notice.dismissible()) {
        return false;
    }
    PlayerLevelUpNoticeLayout layout;
    return buildPlayerLevelUpNoticeLayout(
               notice, *font, layout) &&
           playerLevelUpNoticeContains(
               layout, pointer_x, pointer_y);
}

bool dismissPlayerLevelUpNoticeAtPointer(
    bool pointer_pressed,
    std::int32_t pointer_x,
    std::int32_t pointer_y,
    const gapi::NjpImage* font,
    WorldScene& world) {
    if (!pointer_pressed ||
        !playerLevelUpNoticeAcceptsPointer(
            world.levelUpNotice(),
            pointer_x,
            pointer_y,
            font)) {
        return false;
    }

    world.dismissLevelUpNotice();
    return true;
}

}  // namespace osf
