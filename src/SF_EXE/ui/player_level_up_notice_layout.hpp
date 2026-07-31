#ifndef OPENSHADOWFLARE_PLAYER_LEVEL_UP_NOTICE_LAYOUT_HPP
#define OPENSHADOWFLARE_PLAYER_LEVEL_UP_NOTICE_LAYOUT_HPP

#include <cstdint>

namespace osf {

struct PlayerLevelUpNotice;

namespace gapi {
class NjpImage;
}

struct PlayerLevelUpNoticeLayout {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::int32_t text_x = 0;
    std::int32_t text_y = 0;
};

bool buildPlayerLevelUpNoticeLayout(
    const PlayerLevelUpNotice& notice,
    const gapi::NjpImage& font,
    PlayerLevelUpNoticeLayout& layout);

bool playerLevelUpNoticeContains(
    const PlayerLevelUpNoticeLayout& layout,
    std::int32_t screen_x,
    std::int32_t screen_y);

}  // namespace osf

#endif
