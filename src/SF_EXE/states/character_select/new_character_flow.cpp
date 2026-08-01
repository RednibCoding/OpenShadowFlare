#include "character_select_flow.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace osf::character_select {
namespace {

constexpr std::int32_t kFullBrightness = 1000;

struct MenuRectangle {
    std::int32_t left;
    std::int32_t right;
    std::int32_t top;
    std::int32_t bottom;
};

constexpr MenuRectangle kSavedBackRectangle{
    0x188, 0x1d2, 0x1c2, 0x1cf};
constexpr MenuRectangle kSavedExitRectangle{
    0x23f, 0x279, 7, 0x14};
constexpr MenuRectangle kNewCharacterMaleRectangle{
    0xa7, 0x12f, 0x6a, 0x17b};
constexpr MenuRectangle kNewCharacterFemaleRectangle{
    0x185, 0x1ea, 0x6a, 0x177};
constexpr MenuRectangle kNewCharacterNameConfirmRectangle{
    0x237, 0x25b, 0x1c3, 0x1ce};

bool isInside(
    const MenuRectangle& rectangle,
    std::int32_t x,
    std::int32_t y) {
    return x > rectangle.left && x < rectangle.right &&
           y > rectangle.top && y < rectangle.bottom;
}

void appendTextLimited(
    std::string& destination,
    std::string_view input,
    std::size_t maximum_bytes) {
    for (std::size_t index = 0; index < input.size();) {
        const std::uint8_t first =
            static_cast<std::uint8_t>(input[index]);
        std::size_t length = 1;
        if ((first & 0xe0u) == 0xc0u) {
            length = 2;
        } else if ((first & 0xf0u) == 0xe0u) {
            length = 3;
        } else if ((first & 0xf8u) == 0xf0u) {
            length = 4;
        }
        if (index + length > input.size() ||
            destination.size() + length > maximum_bytes) {
            break;
        }
        destination.append(input.substr(index, length));
        index += length;
    }
}

void eraseLastTextCharacter(std::string& text) {
    if (text.empty()) {
        return;
    }
    std::size_t index = text.size() - 1;
    while (index > 0 &&
           (static_cast<std::uint8_t>(text[index]) & 0xc0u) ==
               0x80u) {
        --index;
    }
    text.erase(index);
}

void beginNameEntry(
    CharacterSelectStateData& data,
    CharacterSelectFrameResult& result) {
    data.character_gender = data.dialog_selection == 0 ? 0 : 1;
    data.character_transition_counter = 1000;
    data.screen = 1;
    data.name_entry_active = true;
    data.input_latch = 1;
    data.brightness_increasing = 1;
    result.character_transition_counter = 1000;
    result.mode_action =
        CharacterSelectModeAction::begin_name_entry;
    ++result.play_selection_sound_count;
}

void updateTextEntry(
    std::string& text,
    const CharacterSelectFrameInput& input) {
    if (input.backspace_pressed) {
        eraseLastTextCharacter(text);
    }
    appendTextLimited(text, input.text_input, 15);
}

void updateNewCharacterModeImpl(
    CharacterSelectStateData& data,
    const CharacterSelectFrameInput& input,
    CharacterSelectFrameResult& result) {
    if (data.fade_target > data.fade_value) {
        data.fade_target = data.fade_value;
    }
    if (data.fade_steps_remaining == 0) {
        const std::int32_t phase = data.launch_counter % 1000;
        result.mode_brightness =
            data.launch_counter == 0
                ? kFullBrightness
                : std::max<std::int32_t>(
                      0,
                      kFullBrightness - phase * 50);
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
        result.mode_brightness = data.fade_target;
    }

    result.character_transition_counter =
        data.character_transition_counter;
    if (data.character_transition_counter >= 1000 &&
        data.character_transition_counter < 1020) {
        ++data.character_transition_counter;
    } else if (data.character_transition_counter == 1020) {
        data.character_transition_counter = 0;
    } else if (
        data.character_transition_counter >= 2000 &&
        data.character_transition_counter < 2020) {
        ++data.character_transition_counter;
    } else if (data.character_transition_counter == 2020) {
        data.character_transition_counter = 0;
    }

    if (data.screen == 1 && data.name_entry_active) {
        updateTextEntry(data.character_name, input);
        if (input.back_pressed ||
            (input.pointer_primary_pressed &&
             isInside(
                 kSavedBackRectangle,
                 input.pointer_x,
                 input.pointer_y))) {
            data.name_entry_active = false;
            data.screen = 0;
            data.input_latch = 1;
            data.character_transition_counter = 2000;
            result.character_transition_counter = 2000;
            result.mode_action =
                CharacterSelectModeAction::cancel_name_entry;
            ++result.play_selection_sound_count;
            return;
        }
        const bool confirmByPointer =
            input.pointer_primary_pressed &&
            isInside(
                kNewCharacterNameConfirmRectangle,
                input.pointer_x,
                input.pointer_y);
        if ((input.confirm_pressed || confirmByPointer) &&
            !data.character_name.empty()) {
            data.name_entry_active = false;
            data.screen = 10;
            data.dialog_selection = 0;
            data.dialog_input_armed = 1;
            data.input_latch = 1;
            result.mode_action =
                CharacterSelectModeAction::accept_character_name;
            ++result.play_selection_sound_count;
        }
        return;
    }

    if (data.screen != 0) {
        return;
    }
    if (data.input_latch == 0 &&
        data.fade_steps_remaining == 0) {
        const std::int32_t previous = data.dialog_selection;
        if (input.up_pressed || input.left_pressed) {
            data.dialog_selection = 0;
            data.dialog_input_armed = 1;
        }
        if (input.down_pressed || input.right_pressed) {
            data.dialog_selection = 1;
            data.dialog_input_armed = 1;
        }
        if (data.dialog_selection != previous) {
            ++result.play_move_sound_count;
        }
    }

    const bool pointerMoved =
        input.pointer_x != data.previous_pointer_x ||
        input.pointer_y != data.previous_pointer_y;
    for (std::int32_t gender = 0; gender < 2; ++gender) {
        const MenuRectangle& rectangle =
            gender == 0
                ? kNewCharacterMaleRectangle
                : kNewCharacterFemaleRectangle;
        if (pointerMoved &&
            isInside(
                rectangle,
                input.pointer_x,
                input.pointer_y)) {
            data.dialog_input_armed = 0;
            if (data.dialog_selection != gender) {
                data.dialog_selection = gender;
                ++result.play_move_sound_count;
            }
        }
        if (data.input_latch == 0 &&
            input.pointer_primary_pressed &&
            isInside(
                rectangle,
                input.pointer_x,
                input.pointer_y)) {
            data.dialog_selection = gender;
            beginNameEntry(data, result);
            break;
        }
    }
    if (data.screen == 0 &&
        data.input_latch == 0 &&
        input.confirm_pressed) {
        beginNameEntry(data, result);
    }

    if (data.launch_counter == 0 &&
        data.input_latch == 0 &&
        (input.back_pressed ||
         (input.pointer_primary_pressed &&
          isInside(
              kSavedBackRectangle,
              input.pointer_x,
              input.pointer_y)))) {
        data.launch_counter = 1000;
        result.mode_action =
            CharacterSelectModeAction::start_back_transition;
        ++result.play_selection_sound_count;
    }
    if (data.launch_counter == 0 &&
        data.input_latch == 0 &&
        input.pointer_primary_pressed &&
        isInside(
            kSavedExitRectangle,
            input.pointer_x,
            input.pointer_y)) {
        data.launch_counter = 2000;
        result.mode_action =
            CharacterSelectModeAction::start_exit_transition;
        ++result.play_selection_sound_count;
    }
    data.previous_pointer_x = input.pointer_x;
    data.previous_pointer_y = input.pointer_y;
}

}  // namespace

void updateNewCharacterMode(
    CharacterSelectStateData& data,
    const CharacterSelectFrameInput& input,
    CharacterSelectFrameResult& result) {
    updateNewCharacterModeImpl(data, input, result);
}

}  // namespace osf::character_select

