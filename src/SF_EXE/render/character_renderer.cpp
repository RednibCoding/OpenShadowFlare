#include "character_renderer.hpp"

#include "gapi/gapi.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace osf {

void renderCharacterAnimationPass(
    gapi::Backend& renderer,
    const gapi::CafAnimation& animation,
    const gapi::NjpImage& patterns,
    const gapi::NjpImage& shadow_patterns,
    WorldPosition position,
    std::int32_t chart_index,
    std::int32_t direction_index,
    std::int32_t animation_frame,
    const CharacterPartEnabled& part_enabled,
    const CharacterPartColor& part_color,
    std::int32_t camera_x,
    std::int32_t camera_y,
    bool shadow,
    std::int32_t shadow_opacity,
    std::int32_t screen_height,
    std::int32_t opacity,
    std::int32_t additional_status) {
    if (animation.charts().empty()) {
        return;
    }
    const std::int32_t selected_chart_index =
        std::clamp(
            chart_index,
            std::int32_t{0},
            static_cast<std::int32_t>(
                animation.charts().size() - 1));
    const gapi::CafChart& chart =
        animation.charts()[
            static_cast<std::size_t>(selected_chart_index)];
    if (direction_index < 0 ||
        static_cast<std::size_t>(direction_index) >=
            chart.directions.size()) {
        return;
    }
    const gapi::CafDirection& direction =
        chart.directions[
            static_cast<std::size_t>(direction_index)];
    if (direction.frame_count <= 0 ||
        direction.parts.empty()) {
        return;
    }
    if ((chart.status & 1) != 0) {
        animation_frame %= direction.frame_count;
    }
    if (animation_frame < 0 ||
        animation_frame >= direction.frame_count) {
        animation_frame = 0;
    }

    struct OrderedCell {
        const gapi::CafCell* cell = nullptr;
        std::size_t part = 0;
    };
    std::vector<OrderedCell> ordered(direction.parts.size());
    for (std::size_t part_index = 0;
         part_index < direction.parts.size();
         ++part_index) {
        if (!part_enabled(part_index)) {
            continue;
        }
        const std::vector<gapi::CafCell>& part =
            direction.parts[part_index];
        if (static_cast<std::size_t>(animation_frame) >=
            part.size()) {
            continue;
        }
        const gapi::CafCell& cell =
            part[static_cast<std::size_t>(animation_frame)];
        if (cell.priority >= 0 &&
            static_cast<std::size_t>(cell.priority) <
                ordered.size()) {
            ordered[
                static_cast<std::size_t>(cell.priority)] = {
                    &cell,
                    part_index,
                };
        }
    }

    for (std::size_t priority = ordered.size();
         priority != 0;
         --priority) {
        const OrderedCell& ordered_cell =
            ordered[priority - 1];
        const gapi::CafCell* cell = ordered_cell.cell;
        if (!cell || cell->pattern_index < 0 ||
            (shadow && (cell->status & 8) == 0)) {
            continue;
        }
        const std::int32_t display_status =
            cell->status | additional_status;
        const CharacterColorStrength strength =
            part_color(ordered_cell.part);
        const ScreenPosition screen_position =
            calculateRealPosition(position);
        const std::int32_t palette =
            shadow || animation.palette_mode() == 0
                ? -1
                : animation.chart_priority_stride() *
                          selected_chart_index +
                      cell->priority;
        renderer.drawPattern(
            shadow
                ? shadow_patterns
                : patterns,
            static_cast<std::size_t>(cell->pattern_index),
            {screen_position.x - camera_x,
             screen_position.y - camera_y - screen_height,
             1000,
             1000,
             1000,
             shadow
                 ? std::clamp(shadow_opacity, std::int32_t{0}, std::int32_t{1000})
                 : std::clamp<std::int32_t>(
                       cell->transparency *
                           std::clamp(opacity, std::int32_t{0}, std::int32_t{1000}) /
                           1000,
                       0,
                       1000),
             shadow ? 1000 : strength.red,
             shadow ? 1000 : strength.green,
             shadow ? 1000 : strength.blue,
             palette,
             {},
             !shadow && (display_status & 0x10) != 0
                 ? gapi::PatternBlendMode::additive
                 : gapi::PatternBlendMode::normal});
    }
}

}  // namespace osf
