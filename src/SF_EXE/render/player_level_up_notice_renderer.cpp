#include "player_level_up_notice_renderer.hpp"

#include "gapi/gapi.hpp"
#include "ui/player_level_up_notice_layout.hpp"
#include "world/player_level_up_notice.hpp"

namespace osf {

void renderPlayerLevelUpNotice(
    gapi::Backend& renderer,
    const PlayerLevelUpNotice& notice,
    const gapi::NjpImage& font) {
    PlayerLevelUpNoticeLayout layout;
    if (!buildPlayerLevelUpNoticeLayout(
            notice, font, layout)) {
        return;
    }

    constexpr gapi::Color kBackground{
        0, 0, 0, 255,
    };
    constexpr gapi::Color kBorder{
        255, 255, 255, 255,
    };
    renderer.drawRectangle({
        layout.x,
        layout.y,
        layout.width,
        layout.height,
        kBackground,
        1000,
        250,
    });
    renderer.drawRectangle({
        layout.x - 1,
        layout.y - 1,
        layout.width + 1,
        1,
        kBorder,
        1000,
        500,
    });
    renderer.drawRectangle({
        layout.x - 1,
        layout.y - 1,
        1,
        layout.height + 1,
        kBorder,
        1000,
        500,
    });
    renderer.drawRectangle({
        layout.x + layout.width - 1,
        layout.y - 1,
        1,
        layout.height + 1,
        kBorder,
        1000,
        500,
    });
    renderer.drawRectangle({
        layout.x - 1,
        layout.y + layout.height - 1,
        layout.width + 1,
        1,
        kBorder,
        1000,
        500,
    });
    renderer.drawText(
        font,
        notice.text,
        {
            layout.text_x,
            layout.text_y,
            {224, 224, 224, 255},
        });
}

}  // namespace osf
