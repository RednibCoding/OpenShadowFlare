#include "gameplay_options_menu.hpp"

#include <algorithm>
#include <cstddef>

namespace osf {
namespace {

bool inside(
    std::int32_t x,
    std::int32_t y,
    std::int32_t left,
    std::int32_t top,
    std::int32_t right,
    std::int32_t bottom) {
    return x >= left && x < right &&
           y >= top && y < bottom;
}

bool setBooleanRow(
    const GameplayOptionsInput& input,
    std::int32_t top,
    bool& value) {
    if (!input.pointer_primary_pressed ||
        input.pointer_y < top ||
        input.pointer_y >= top + 12) {
        return false;
    }
    if (input.pointer_x >= 376 &&
        input.pointer_x < 426) {
        value = true;
        return true;
    }
    if (input.pointer_x >= 426 &&
        input.pointer_x < 464) {
        value = false;
        return true;
    }
    return false;
}

bool selectClickRange(
    const GameplayOptionsInput& input,
    GameConfig& config) {
    if (!input.pointer_primary_pressed ||
        input.pointer_y < 182 ||
        input.pointer_y >= 194) {
        return false;
    }
    for (std::int32_t index = 0; index < 5; ++index) {
        const std::int32_t left = 317 + index * 30;
        if (input.pointer_x >= left &&
            input.pointer_x < left + 24) {
            config.click_range = index;
            return true;
        }
    }
    return false;
}

bool moveClickPriorityToEnd(
    const GameplayOptionsInput& input,
    GameConfig& config) {
    if (!input.pointer_primary_pressed ||
        input.pointer_y < 198 ||
        input.pointer_y >= 210) {
        return false;
    }
    std::int32_t display_index = -1;
    for (std::int32_t index = 0; index < 5; ++index) {
        const std::int32_t left = 317 + index * 30;
        if (input.pointer_x >= left &&
            input.pointer_x < left + 24) {
            display_index = index;
            break;
        }
    }
    if (display_index < 0) {
        return false;
    }

    const std::int32_t selected_priority =
        4 - display_index;
    const auto selected = std::find(
        config.click_priority.begin(),
        config.click_priority.end(),
        selected_priority);
    if (selected == config.click_priority.end()) {
        return false;
    }
    for (std::int32_t& priority :
         config.click_priority) {
        if (priority < selected_priority) {
            ++priority;
        }
    }
    *selected = 0;
    return true;
}

bool setVolume(
    const GameplayOptionsInput& input,
    std::int32_t top,
    std::int32_t& volume) {
    if (!input.pointer_primary_down ||
        !inside(
            input.pointer_x,
            input.pointer_y,
            176,
            top,
            464,
            top + 16)) {
        return false;
    }
    const std::int32_t selected =
        gameplayOptionsVolumeFromPointerX(
            input.pointer_x);
    if (selected == volume) {
        return false;
    }
    volume = selected;
    return true;
}

}  // namespace

GameplayOptionsResult GameplayOptionsMenu::update(
    const GameplayOptionsInput& input,
    GameConfig& config) {
    pointer_x_ = input.pointer_x;
    pointer_y_ = input.pointer_y;
    ++animation_counter_;

    if (input.toggle_pressed) {
        active_ = !active_;
        page_ = GameplayOptionsPage::settings;
        return {};
    }
    if (!active_) {
        return {};
    }

    GameplayOptionsResult result;
    if (page_ == GameplayOptionsPage::saving) {
        return result;
    }
    if (page_ != GameplayOptionsPage::settings) {
        if (!input.pointer_primary_pressed) {
            return result;
        }
        if (inside(
                input.pointer_x,
                input.pointer_y,
                336,
                202,
                360,
                214)) {
            result.play_confirm_sound = true;
            result.action =
                page_ ==
                        GameplayOptionsPage::
                            return_to_title_confirmation
                    ? GameplayOptionsAction::
                          save_and_return_to_title
                    : GameplayOptionsAction::save_and_exit;
            page_ = GameplayOptionsPage::saving;
        } else if (inside(
                       input.pointer_x,
                       input.pointer_y,
                       384,
                       202,
                       420,
                       214)) {
            page_ = GameplayOptionsPage::settings;
            result.play_click_sound = true;
        }
        return result;
    }

    if (input.pointer_primary_pressed &&
        inside(
            input.pointer_x,
            input.pointer_y,
            176,
            302,
            464,
            314)) {
        page_ =
            GameplayOptionsPage::return_to_title_confirmation;
        result.play_confirm_sound = true;
        return result;
    }
    if (input.pointer_primary_pressed &&
        inside(
            input.pointer_x,
            input.pointer_y,
            176,
            318,
            464,
            330)) {
        page_ = GameplayOptionsPage::exit_game_confirmation;
        result.play_confirm_sound = true;
        return result;
    }

    const bool clicked_setting =
        setBooleanRow(
            input, 102, config.semi_transparent_objects) ||
        setBooleanRow(
            input, 118, config.semi_transparent_shadow) ||
        setBooleanRow(
            input, 134, config.display_darkness) ||
        setBooleanRow(
            input, 150, config.save_image_at_game_end) ||
        setBooleanRow(
            input, 166, config.click_range_enabled) ||
        selectClickRange(input, config) ||
        moveClickPriorityToEnd(input, config);
    const bool volume_changed =
        setVolume(input, 218, config.effect_volume) ||
        setVolume(input, 238, config.bgm_volume);
    result.config_changed =
        clicked_setting || volume_changed;
    result.play_click_sound = clicked_setting;
    return result;
}

void GameplayOptionsMenu::restoreConfirmation(
    GameplayOptionsAction action) {
    if (page_ != GameplayOptionsPage::saving) {
        return;
    }
    page_ =
        action ==
                GameplayOptionsAction::save_and_return_to_title
            ? GameplayOptionsPage::
                  return_to_title_confirmation
            : GameplayOptionsPage::exit_game_confirmation;
}

void GameplayOptionsMenu::close() {
    active_ = false;
    page_ = GameplayOptionsPage::settings;
}

bool GameplayOptionsMenu::active() const {
    return active_;
}

GameplayOptionsPage GameplayOptionsMenu::page() const {
    return page_;
}

std::int32_t GameplayOptionsMenu::pointerX() const {
    return pointer_x_;
}

std::int32_t GameplayOptionsMenu::pointerY() const {
    return pointer_y_;
}

std::int32_t GameplayOptionsMenu::animationCounter() const {
    return animation_counter_;
}

std::int32_t gameplayOptionsVolumeFromPointerX(
    std::int32_t pointer_x) {
    std::int32_t slider_position = pointer_x - 252;
    if (slider_position <= 0) {
        return -10000;
    }
    slider_position =
        std::min<std::int32_t>(slider_position, 200);
    return slider_position * 3000 / 200 - 3000;
}

std::int32_t gameplayOptionsVolumeSliderOffset(
    std::int32_t volume) {
    if (volume <= -10000) {
        return 0;
    }
    volume = std::clamp<std::int32_t>(volume, -3000, 0);
    return ((volume * 5 + 15000) * 40) / 3000;
}

}  // namespace osf
