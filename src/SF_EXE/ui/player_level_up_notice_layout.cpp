#include "player_level_up_notice_layout.hpp"

#include "conversation_layout.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "world/player_level_up_notice.hpp"

#include <algorithm>

namespace osf {
namespace {

constexpr std::int32_t kScreenWidth = 640;
constexpr std::int32_t kWorldAreaHeight = 416;
constexpr std::int32_t kTextPadding = 4;
constexpr std::int32_t kSlideStartCounter = 840;
constexpr std::int32_t kSlideUpdates = 10;

}  // namespace

bool buildPlayerLevelUpNoticeLayout(
    const PlayerLevelUpNotice& notice,
    const gapi::NjpImage& font,
    PlayerLevelUpNoticeLayout& layout) {
    layout = {};
    if (!notice.active() || font.patterns().empty()) {
        return false;
    }

    const gapi::NjpPattern& base_pattern =
        font.patterns().front();
    const std::int32_t cell_width =
        base_pattern.width / 16;
    const std::int32_t cell_height =
        base_pattern.height / 16;
    if (cell_width < 1 || cell_height < 1) {
        return false;
    }

    layout.width =
        bitmapTextPixelWidth(
            notice.text, cell_width) +
        kTextPadding * 2;
    layout.height =
        bitmapTextLineCount(notice.text) *
            cell_height +
        kTextPadding * 2;
    if (layout.width <= kTextPadding * 2 ||
        layout.height <= kTextPadding * 2) {
        layout = {};
        return false;
    }

    const std::int32_t centered_x =
        (kScreenWidth - layout.width) / 2;
    const std::int32_t centered_y =
        (kWorldAreaHeight - layout.height) / 2;
    layout.x = centered_x;
    layout.y = centered_y;

    if (notice.counter < kSlideStartCounter) {
        const std::int32_t slide_update =
            kSlideStartCounter - notice.counter;
        layout.x =
            centered_x +
            ((kScreenWidth - centered_x -
              layout.width) *
             slide_update) /
                kSlideUpdates;
        layout.y =
            centered_y -
            (centered_y * slide_update) /
                kSlideUpdates;
        layout.x = std::min(
            layout.x, kScreenWidth - layout.width);
        layout.y = std::max(layout.y, 1);
    }

    layout.text_x = layout.x + kTextPadding;
    layout.text_y = layout.y + kTextPadding;
    return true;
}

bool playerLevelUpNoticeContains(
    const PlayerLevelUpNoticeLayout& layout,
    std::int32_t screen_x,
    std::int32_t screen_y) {
    return screen_x >= layout.x &&
           screen_x < layout.x + layout.width &&
           screen_y >= layout.y &&
           screen_y < layout.y + layout.height;
}

}  // namespace osf
