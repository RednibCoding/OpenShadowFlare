#include "enemy_nameplate_renderer.hpp"

#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "ui/conversation_layout.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

namespace osf {

void renderEnemyNameplate(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    const gapi::NjpImage* status_icons,
    const EnemyNameplate& nameplate) {
    const std::string label =
        std::string("\x81\x40") +
        std::string(nameplate.name);
    const std::int32_t half_width =
        bitmapTextPixelWidth(label, 6) / 2;
    const std::int32_t inner_x =
        nameplate.center_x - half_width - 4;
    const std::int32_t inner_width =
        half_width * 2 + 6;

    // FUN_0040ee70 places a dark frame around a translucent life bar whose
    // width follows the name.
    renderer.drawRectangle({
        nameplate.center_x - half_width - 5,
        nameplate.y - 3,
        half_width * 2 + 8,
        18,
        {0, 0, 0, 255},
        1000,
        800,
    });
    const std::int32_t maximum_life =
        std::max(nameplate.maximum_life, std::int32_t{0});
    const std::int32_t current_life =
        std::clamp(
            nameplate.current_life, std::int32_t{0}, maximum_life);
    const std::int32_t life_width =
        maximum_life == 0
            ? 0
            : inner_width * current_life /
                  maximum_life;
    if (life_width > 0) {
        renderer.drawRectangle({
            inner_x,
            nameplate.y - 2,
            life_width,
            16,
            {128, 32, 32, 255},
            1000,
            500,
        });
    }
    if (life_width < inner_width) {
        renderer.drawRectangle({
            inner_x + life_width,
            nameplate.y - 2,
            inner_width - life_width,
            16,
            {0, 0, 0, 255},
            1000,
            500,
        });
    }
    renderer.drawText(
        font,
        label,
        {
            nameplate.center_x - half_width + 1,
            nameplate.y + 1,
            {0, 0, 0, 255},
        });
    renderer.drawText(
        font,
        label,
        {
            nameplate.center_x - half_width,
            nameplate.y,
            nameplate.name_color,
        });
    const std::int32_t element_pattern =
        nameplate.native_element + 3;
    if (status_icons && element_pattern >= 0) {
        renderer.drawPattern(
            *status_icons,
            static_cast<std::size_t>(element_pattern),
            {
                nameplate.center_x - half_width - 2,
                nameplate.y + 1,
            });
    }
}

}  // namespace osf
