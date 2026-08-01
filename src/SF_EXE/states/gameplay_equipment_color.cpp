#include "gameplay_equipment_color.hpp"

#include "items/item_appearance.hpp"

#include <array>
#include <cstddef>

namespace osf {

void GameplayEquipmentColor::open(
    const std::array<bool, target_count>& available,
    const std::array<std::int32_t, target_count>& colors) {
    active_ = true;
    available_ = available;
    colors_ = colors;
    original_colors_ = colors;
    selected_target_ = EquipmentColorTarget::none;
    for (std::size_t index = 0; index < available_.size(); ++index) {
        if (available_[index]) {
            selected_target_ = static_cast<EquipmentColorTarget>(index);
            break;
        }
    }
    preview_chart_ = 7;
    accept_hovered_ = false;
    cancel_hovered_ = false;
}

void GameplayEquipmentColor::close() {
    active_ = false;
    accept_hovered_ = false;
    cancel_hovered_ = false;
}

GameplayEquipmentColorResult GameplayEquipmentColor::update(
    const GameplayEquipmentColorInput& input) {
    GameplayEquipmentColorResult result;
    if (!active_) {
        return result;
    }
    accept_hovered_ = inside(
        input.pointer_x, input.pointer_y, 198, 300, 241, 317);
    cancel_hovered_ = inside(
        input.pointer_x, input.pointer_y, 335, 300, 452, 317);
    result.pointer_consumed =
        input.pointer_primary_pressed && input.pointer_y < 412;
    if (input.cancel_pressed) {
        result.cancelled = true;
        close();
        return result;
    }
    if (!input.pointer_primary_pressed) {
        return result;
    }

    if (accept_hovered_) {
        result.accepted = true;
        result.play_move_sound = true;
        close();
        return result;
    }
    if (cancel_hovered_) {
        result.cancelled = true;
        result.play_move_sound = true;
        close();
        return result;
    }

    constexpr std::array<std::int32_t, target_count> tops{{
        199, 223, 247,
    }};
    for (std::size_t index = 0; index < tops.size(); ++index) {
        if (available_[index] &&
            inside(
                input.pointer_x,
                input.pointer_y,
                174,
                tops[index],
                215,
                tops[index] + 16)) {
            selected_target_ =
                static_cast<EquipmentColorTarget>(index);
            result.play_move_sound = true;
            return result;
        }
    }

    const std::int32_t color = colorAt(
        input.pointer_x, input.pointer_y);
    if (color >= 0) {
        const std::size_t target =
            static_cast<std::size_t>(selected_target_);
        if (target < target_count && available_[target]) {
            colors_[target] = color;
            result.color_changed = true;
            result.target = selected_target_;
            result.color = color;
        }
        return result;
    }
    if (inside(
            input.pointer_x,
            input.pointer_y,
            400,
            280,
            464,
            296)) {
        const std::size_t target =
            static_cast<std::size_t>(selected_target_);
        if (target < target_count && available_[target]) {
            colors_[target] = -1;
            result.color_changed = true;
            result.target = selected_target_;
            result.color = -1;
        }
        result.play_move_sound = true;
        return result;
    }

    constexpr std::array<std::int32_t, 9> charts{{
        5, 4, 3,
        6, -1, 2,
        7, 0, 1,
    }};
    for (std::size_t row = 0; row < 3; ++row) {
        for (std::size_t column = 0; column < 3; ++column) {
            const std::size_t index = row * 3 + column;
            if (charts[index] < 0) {
                continue;
            }
            if (inside(
                    input.pointer_x,
                    input.pointer_y,
                    216 + static_cast<std::int32_t>(column) * 33,
                    180 + static_cast<std::int32_t>(row) * 31,
                    249 + static_cast<std::int32_t>(column) * 33,
                    211 + static_cast<std::int32_t>(row) * 31)) {
                preview_chart_ = charts[index];
                return result;
            }
        }
    }
    return result;
}

bool GameplayEquipmentColor::active() const {
    return active_;
}

EquipmentColorTarget GameplayEquipmentColor::selectedTarget() const {
    return selected_target_;
}

std::int32_t GameplayEquipmentColor::selectedColor() const {
    const std::size_t target =
        static_cast<std::size_t>(selected_target_);
    return target < target_count ? colors_[target] : -1;
}

std::int32_t GameplayEquipmentColor::previewChart() const {
    return preview_chart_;
}

bool GameplayEquipmentColor::acceptHovered() const {
    return accept_hovered_;
}

bool GameplayEquipmentColor::cancelHovered() const {
    return cancel_hovered_;
}

const std::array<std::int32_t, GameplayEquipmentColor::target_count>&
GameplayEquipmentColor::originalColors() const {
    return original_colors_;
}

bool GameplayEquipmentColor::inside(
    std::int32_t x,
    std::int32_t y,
    std::int32_t left,
    std::int32_t top,
    std::int32_t right,
    std::int32_t bottom) {
    return x >= left && x < right && y >= top && y < bottom;
}

std::int32_t GameplayEquipmentColor::colorAt(
    std::int32_t pointer_x,
    std::int32_t pointer_y) const {
    for (std::int32_t row = 0; row < 4; ++row) {
        for (std::int32_t column = 0; column < 4; ++column) {
            if (inside(
                    pointer_x,
                    pointer_y,
                    332 + column * 32,
                    187 + row * 24,
                    358 + column * 32,
                    205 + row * 24)) {
                return row * 4 + column;
            }
        }
    }
    return -1;
}

}  // namespace osf
