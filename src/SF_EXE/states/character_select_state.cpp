#include "character_select_state.hpp"

#include "character_select/character_select_flow.hpp"
#include "save_catalog.hpp"

#include <utility>

namespace osf {
namespace {

constexpr std::int32_t kMenuMusicSlot = 500;

constexpr std::array<std::int32_t, 58> kCharacterSelectInputBindings{{
    1, 2, 16, 17, 38, 40, 37, 39, 9, 27, 13, 46,
    65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75,
    76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86,
    87, 88, 89, 90, 48, 49, 50, 51, 52, 53, 54,
    55, 56, 57, 96, 97, 98, 99, 100, 101, 102,
    103, 104, 105,
}};

bool loadPattern(
    const std::function<bool(std::int32_t, std::string_view)>& callback,
    std::int32_t id,
    std::string_view path) {
    return !callback || callback(id, path);
}

bool pointerInside(
    const CharacterSelectFrameInput& input,
    std::int32_t left,
    std::int32_t right,
    std::int32_t top,
    std::int32_t bottom) {
    return input.pointer_x > left &&
           input.pointer_x < right &&
           input.pointer_y > top &&
           input.pointer_y < bottom;
}

void updateVisualState(
    CharacterSelectFrameResult& result,
    const CharacterSelectFrameInput& input) {
    result.host_connect_hovered =
        pointerInside(input, 0x171, 0x196, 0x114, 0x120);
    result.host_back_hovered =
        pointerInside(input, 0xe9, 0x132, 0x114, 0x120);
    result.host_paste_hovered =
        pointerInside(input, 0x175, 0x189, 0xe3, 0xf5);
    result.name_confirm_hovered =
        pointerInside(input, 0x237, 0x25b, 0x1c3, 0x1ce);
    for (std::size_t index = 0;
         index < result.save_slot_hovered.size();
         ++index) {
        const std::int32_t x =
            32 + static_cast<std::int32_t>(index % 2) * 304;
        const std::int32_t y =
            188 + static_cast<std::int32_t>(index / 2) * 88;
        result.save_slot_hovered[index] =
            pointerInside(input, x, x + 287, y, y + 76);
    }
}

}  // namespace


CharacterSelectState::CharacterSelectState(
    CharacterSelectStateHooks hooks)
    : hooks_(std::move(hooks)) {}

void CharacterSelectState::enter(std::int32_t retail_argument) {
    if (hooks_.begin_scene) {
        hooks_.begin_scene();
    }
    // Like retail function 0x00421920, failure is not checked here.
    loadPattern(
        hooks_.load_pattern, 4,
        "System\\Select\\Pattern\\Select.njp");
    if (hooks_.configure_input) {
        hooks_.configure_input(
            kCharacterSelectInputBindings.data(),
            kCharacterSelectInputBindings.size());
    }

    if (data_.mode == CharacterSelectMode::new_character &&
        data_.new_character_data_loaded) {
        if (hooks_.release_new_character) {
            hooks_.release_new_character();
        }
        data_.new_character_data_loaded = false;
    }

    data_.screen = 0;
    data_.input_latch = 1;
    if (retail_argument == 0) {
        data_.mode = CharacterSelectMode::new_character;
        data_.new_character_data_loaded = true;
        data_.next_save_path =
            findNextRetailSavePath(hooks_.file_exists).path;
        if (hooks_.prepare_new_character) {
            hooks_.prepare_new_character(data_.next_save_path);
        }
        data_.character_gender = 0;
        data_.character_name.clear();
        data_.name_entry_active = false;
        data_.host_address.clear();
        data_.host_entry_active = false;
        data_.dialog_selection = 0;
        data_.dialog_input_armed = 1;
    } else {
        data_.mode = CharacterSelectMode::saved_game;
        data_.next_save_path.clear();
        if (hooks_.load_saved_characters) {
            hooks_.load_saved_characters();
        }
        data_.save_hover_animation = 0;
        data_.saved_game_selection = 0;
        data_.dialog_selection = 0;
        data_.dialog_input_armed = 1;
    }

    data_.fade_steps_remaining = 0x14;
    data_.launch_counter = 0;
    data_.character_transition_counter = 0;
    data_.brightness_increasing = 1;
    data_.selection_result = -1;
    data_.selected_saved_game = -1;
    if (hooks_.set_cursor_state) {
        hooks_.set_cursor_state(-1);
    }
    if ((!hooks_.voice_is_playing ||
         !hooks_.voice_is_playing(kMenuMusicSlot)) &&
        hooks_.play_voice) {
        hooks_.play_voice(kMenuMusicSlot, true);
    }
    data_.input_latch = 1;
    data_.active = true;
}

void CharacterSelectState::leave() {
    data_.temporary_buffer.clear();
    if (hooks_.release_voice) {
        hooks_.release_voice(kMenuMusicSlot);
    }
    if (hooks_.clear_scene) {
        hooks_.clear_scene();
    }
    if (hooks_.release_pattern) {
        hooks_.release_pattern(4);
    }
    data_.active = false;
}

CharacterSelectFrameResult CharacterSelectState::update(
    const CharacterSelectFrameInput& input) {
    CharacterSelectFrameResult result;
    result.mode = data_.mode;
    updateVisualState(result, input);
    if (input.input_suspended) {
        result.processed = false;
        return result;
    }

    if (data_.fade_steps_remaining > 0) {
        const std::int32_t brightness =
            (21 - data_.fade_steps_remaining) * 50;
        data_.fade_value = brightness;
        data_.fade_target = brightness;
        --data_.fade_steps_remaining;
    }
    result.background_brightness = data_.fade_value;

    if (data_.brightness_increasing == 0) {
        if (data_.fade_target > 500) {
            data_.fade_target -= 80;
        }
    } else if (
        data_.fade_steps_remaining == 0 &&
        data_.fade_target < 1000) {
        data_.fade_target += 80;
    }

    const bool valid_mode =
        data_.mode == CharacterSelectMode::new_character ||
        data_.mode == CharacterSelectMode::saved_game;
    if (valid_mode) {
        data_.rendered_mode = data_.mode;

        if (data_.mode == CharacterSelectMode::saved_game &&
            data_.screen == 1) {
            data_.input_latch = 1;
            if (!character_select::updateSavedGameDeleteDialog(
                    data_,
                    input,
                    result,
                    hooks_.delete_saved_character)) {
                data_.brightness_increasing = 1;
                data_.screen = 0;
                data_.input_latch = 1;
            }
        }

        // Both retail mode renderers contain this same transition-counter
        // prelude before their mode-specific interaction and drawing.
        bool mode_returned_early = false;
        if (data_.launch_counter == 1022) {
            result.action = CharacterSelectAction::return_to_title;
            mode_returned_early = true;
        } else if (data_.launch_counter == 2022) {
            result.action = CharacterSelectAction::exit_game;
            mode_returned_early = true;
        } else if (data_.launch_counter > 0) {
            ++data_.launch_counter;
        }

        if (!mode_returned_early &&
            data_.mode == CharacterSelectMode::new_character) {
            character_select::updateNewCharacterMode(
                data_, input, result);
        } else if (
            !mode_returned_early &&
            data_.mode == CharacterSelectMode::saved_game) {
            character_select::updateSavedGameMode(
                data_, input, result);
        }

        switch (data_.screen) {
        case 10:
            result.screen_update =
                CharacterSelectScreenUpdate::screen_10;
            character_select::updateGameModeScreen(
                data_, input, result);
            break;
        case 11:
            result.screen_update =
                CharacterSelectScreenUpdate::screen_11;
            character_select::updateNetworkModeScreen(
                data_, input, result);
            break;
        case 12:
            result.screen_update =
                CharacterSelectScreenUpdate::screen_12;
            character_select::updateHostScreen(
                data_, input, result, hooks_.read_clipboard);
            break;
        case 20:
            if (data_.launch_counter == 0) {
                data_.launch_counter = 5010;
            }
            if (data_.launch_counter == 5024) {
                result.action =
                    CharacterSelectAction::enter_gameplay;
                return result;
            }
            break;
        default:
            break;
        }
    }

    if (data_.input_latch == 1) {
        data_.input_latch = 0;
    }
    return result;
}

const CharacterSelectStateData& CharacterSelectState::data() const {
    return data_;
}

CharacterSelectStateData& CharacterSelectState::data() {
    return data_;
}

const std::array<std::int32_t, 58>&
retailCharacterSelectInputBindings() {
    return kCharacterSelectInputBindings;
}

}  // namespace osf
