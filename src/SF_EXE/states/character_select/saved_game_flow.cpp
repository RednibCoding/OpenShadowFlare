#include "character_select_flow.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

namespace osf::character_select {
namespace {

constexpr std::int32_t kFullBrightness = 1000;

struct MenuRectangle {
    std::int32_t left;
    std::int32_t right;
    std::int32_t top;
    std::int32_t bottom;
};

constexpr std::array<MenuRectangle, 6> kSavedGameRectangles{{
    {32, 319, 188, 264},
    {336, 623, 188, 264},
    {32, 319, 276, 352},
    {336, 623, 276, 352},
    {32, 319, 364, 440},
    {336, 623, 364, 440},
}};
constexpr MenuRectangle kSavedBackRectangle{
    0x188, 0x1d2, 0x1c2, 0x1cf};
constexpr MenuRectangle kSavedExitRectangle{
    0x23f, 0x279, 7, 0x14};
constexpr MenuRectangle kSavedContinueRectangle{
    0x236, 0x25c, 0x1c2, 0x1cf};
constexpr MenuRectangle kSavedDeleteRectangle{
    0x3d, 0xa9, 0x1c2, 0x1cf};
constexpr MenuRectangle kDeleteDialogYesRectangle{
    0xf7, 0x129, 0x113, 0x120};
constexpr MenuRectangle kDeleteDialogNoRectangle{
    0x15c, 0x183, 0x113, 0x120};

bool isInside(
    const MenuRectangle& rectangle,
    std::int32_t x,
    std::int32_t y) {
    return x > rectangle.left && x < rectangle.right &&
           y > rectangle.top && y < rectangle.bottom;
}

std::int32_t modeTransitionBrightness(std::int32_t timer) {
    const std::int32_t brightness =
        kFullBrightness - (timer % 1000) * 100;
    return brightness < 0 ? 0 : brightness;
}

}  // namespace

void beginSavedGameChoice(
    CharacterSelectStateData& data,
    CharacterSelectFrameResult& result) {
    data.selected_saved_game = data.saved_game_selection;
    data.screen = 10;
    data.input_latch = 1;
    data.brightness_increasing = 0;
    data.dialog_selection = 0;
    data.dialog_input_armed = 1;
    result.mode_action = CharacterSelectModeAction::choose_saved_game;
    ++result.play_selection_sound_count;
}

void beginSavedGameDelete(
    CharacterSelectStateData& data,
    CharacterSelectFrameResult& result) {
    data.screen = 1;
    data.input_latch = 1;
    data.brightness_increasing = 0;
    data.dialog_selection = 1;
    data.dialog_input_armed = 1;
    result.mode_action =
        CharacterSelectModeAction::open_delete_saved_game_dialog;
    ++result.play_selection_sound_count;
}

bool updateSavedGameDeleteDialog(
    CharacterSelectStateData& data,
    const CharacterSelectFrameInput& input,
    CharacterSelectFrameResult& result,
    const std::function<void(std::int32_t)>& delete_saved_character) {
    if (input.up_pressed || input.left_pressed) {
        data.dialog_input_armed = 1;
        if (data.dialog_selection != 0) {
            data.dialog_selection = 0;
            ++result.play_move_sound_count;
        }
    }
    if (input.down_pressed || input.right_pressed) {
        data.dialog_input_armed = 1;
        if (data.dialog_selection != 1) {
            data.dialog_selection = 1;
            ++result.play_move_sound_count;
        }
    }

    const bool pointer_moved =
        input.pointer_x != data.dialog_previous_pointer_x ||
        input.pointer_y != data.dialog_previous_pointer_y;

    if (isInside(
            kDeleteDialogYesRectangle,
            input.pointer_x,
            input.pointer_y) &&
        data.dialog_input_armed == 1 &&
        pointer_moved) {
        data.dialog_input_armed = 0;
    }
    if (isInside(
            kDeleteDialogYesRectangle,
            input.pointer_x,
            input.pointer_y) &&
        data.dialog_input_armed == 0) {
        data.dialog_selection = 0;
        if (input.pointer_primary_pressed) {
            const std::int32_t deleted_index =
                data.saved_game_selection;
            if (delete_saved_character) {
                delete_saved_character(deleted_index);
            }
            data.saved_game_selection = 0;
            result.mode_action =
                CharacterSelectModeAction::confirm_saved_game_delete;
            ++result.play_selection_sound_count;
            return false;
        }
    }

    if (input.confirm_pressed && data.dialog_selection == 0) {
        const std::int32_t deleted_index =
            data.saved_game_selection;
        if (delete_saved_character) {
            delete_saved_character(deleted_index);
        }
        data.saved_game_selection = 0;
        result.mode_action =
            CharacterSelectModeAction::confirm_saved_game_delete;
        ++result.play_selection_sound_count;
        return false;
    }

    if (isInside(
            kDeleteDialogNoRectangle,
            input.pointer_x,
            input.pointer_y) &&
        data.dialog_input_armed == 1 &&
        pointer_moved) {
        data.dialog_input_armed = 0;
    }
    if (isInside(
            kDeleteDialogNoRectangle,
            input.pointer_x,
            input.pointer_y) &&
        data.dialog_input_armed == 0) {
        data.dialog_selection = 1;
        if (input.pointer_primary_pressed) {
            result.mode_action =
                CharacterSelectModeAction::cancel_saved_game_delete;
            ++result.play_selection_sound_count;
            return false;
        }
    }

    if (input.back_pressed ||
        (input.confirm_pressed && data.dialog_selection == 1)) {
        result.mode_action =
            CharacterSelectModeAction::cancel_saved_game_delete;
        ++result.play_selection_sound_count;
        return false;
    }

    data.dialog_previous_pointer_x = input.pointer_x;
    data.dialog_previous_pointer_y = input.pointer_y;
    return true;
}

void updateSavedGameMode(
    CharacterSelectStateData& data,
    const CharacterSelectFrameInput& input,
    CharacterSelectFrameResult& result) {
    if (data.fade_target > data.fade_value) {
        data.fade_target = data.fade_value;
    }

    if (data.fade_steps_remaining == 0) {
        result.mode_brightness =
            modeTransitionBrightness(data.launch_counter);
        const std::int32_t phase = data.launch_counter % 1000;
        if (phase > 5) {
            data.fade_value =
                kFullBrightness + ((5 - phase) * 1000) / 15;
            if (data.fade_value < 0) {
                data.fade_value = 0;
            }
        }
        if (data.fade_target > data.fade_value) {
            data.fade_target = data.fade_value;
        }
        if (result.mode_brightness > data.fade_target) {
            result.mode_brightness = data.fade_target;
        }
    } else {
        data.pointer_click_cooldown = 0;
        result.mode_brightness = data.fade_target;
    }

    const std::int32_t save_count =
        input.saved_game_count < 0
            ? 0
            : (input.saved_game_count > 6
                   ? 6
                   : input.saved_game_count);
    if (data.screen == 0) {
        const std::int32_t previous_selection =
            data.saved_game_selection;
        if (data.input_latch == 0 && save_count != 0) {
            if (input.up_pressed) {
                data.saved_game_selection -= 2;
                if (data.saved_game_selection < 0) {
                    data.saved_game_selection = 0;
                }
            }
            if (input.down_pressed) {
                data.saved_game_selection += 2;
                if (data.saved_game_selection > save_count - 1) {
                    data.saved_game_selection = save_count - 1;
                }
            }
            if (input.left_pressed) {
                --data.saved_game_selection;
                if (data.saved_game_selection < 0) {
                    data.saved_game_selection = 0;
                }
            }
            if (input.right_pressed) {
                ++data.saved_game_selection;
                if (data.saved_game_selection > save_count - 1) {
                    data.saved_game_selection = save_count - 1;
                }
            }
        }
        if (data.saved_game_selection != previous_selection) {
            ++result.play_move_sound_count;
        }

        if (data.pointer_click_cooldown != 0) {
            --data.pointer_click_cooldown;
        }
        if (input.pointer_primary_pressed && data.input_latch == 0) {
            if (data.pointer_click_cooldown != 0 &&
                data.previous_pointer_x == input.pointer_x &&
                data.previous_pointer_y == input.pointer_y) {
                result.pointer_double_click = true;
                data.pointer_click_cooldown = 0;
                data.previous_pointer_x = -1;
            } else {
                data.pointer_click_cooldown = 10;
                data.previous_pointer_x = input.pointer_x;
                data.previous_pointer_y = input.pointer_y;
            }
        }
    }

    bool save_hovered = false;
    for (std::int32_t index = 0; index < 6; ++index) {
        if (data.brightness_increasing == 0 ||
            data.launch_counter != 0 ||
            data.save_hover_animation < 28 ||
            !isInside(
                kSavedGameRectangles[static_cast<std::size_t>(index)],
                input.pointer_x,
                input.pointer_y)) {
            continue;
        }

        ++data.save_hover_animation;
        save_hovered = true;
        if (data.screen == 0 &&
            input.pointer_primary_pressed &&
            index < save_count &&
            data.input_latch == 0) {
            data.saved_game_selection = index;
            ++result.play_selection_sound_count;
            if (result.pointer_double_click) {
                beginSavedGameChoice(data, result);
            }
        }
    }

    if (data.fade_steps_remaining == 0 &&
        data.save_hover_animation < 28) {
        ++data.save_hover_animation;
    }
    if (data.save_hover_animation >= 28 && !save_hovered) {
        data.save_hover_animation = 64;
    }

    if (data.brightness_increasing != 0 &&
        data.launch_counter == 0) {
        const bool back_highlighted =
            isInside(
                kSavedBackRectangle,
                input.pointer_x,
                input.pointer_y) ||
            (input.back_pressed && data.input_latch == 0);
        if (back_highlighted &&
            data.screen == 0 &&
            (input.pointer_primary_pressed || input.back_pressed) &&
            data.input_latch == 0) {
            data.launch_counter = 1000;
            result.mode_action =
                CharacterSelectModeAction::start_back_transition;
            ++result.play_selection_sound_count;
        }

        const bool exit_highlighted =
            isInside(
                kSavedExitRectangle,
                input.pointer_x,
                input.pointer_y);
        if (data.launch_counter == 0 &&
            exit_highlighted &&
            data.screen == 0 &&
            input.pointer_primary_pressed &&
            data.input_latch == 0) {
            data.launch_counter = 2000;
            result.mode_action =
                CharacterSelectModeAction::start_exit_transition;
            ++result.play_selection_sound_count;
        }
    }

    const bool selected_save_exists =
        data.saved_game_selection >= 0 &&
        data.saved_game_selection < save_count;
    if (!selected_save_exists) {
        return;
    }

    if (data.brightness_increasing != 0 &&
        data.launch_counter == 0) {
        const bool continue_highlighted =
            isInside(
                kSavedContinueRectangle,
                input.pointer_x,
                input.pointer_y) ||
            (input.confirm_pressed && data.input_latch == 0);
        if (continue_highlighted &&
            data.screen == 0 &&
            (input.pointer_primary_pressed || input.confirm_pressed)) {
            beginSavedGameChoice(data, result);
        }

        const bool delete_highlighted =
            isInside(
                kSavedDeleteRectangle,
                input.pointer_x,
                input.pointer_y) ||
            (input.delete_pressed && data.input_latch == 0);
        if (delete_highlighted &&
            data.screen == 0 &&
            (input.pointer_primary_pressed || input.delete_pressed)) {
            beginSavedGameDelete(data, result);
        }
    }
}

}  // namespace osf::character_select

