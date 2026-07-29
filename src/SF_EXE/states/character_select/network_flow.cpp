#include "character_select_flow.hpp"

#include <array>
#include <cstdint>

namespace osf::character_select {
namespace {

struct MenuRectangle {
    std::int32_t left;
    std::int32_t right;
    std::int32_t top;
    std::int32_t bottom;
};

constexpr std::array<MenuRectangle, 3> kCharacterModeRectangles{{
    {0xdb, 0x1a5, 0xcf, 0xdc},
    {0xdd, 0x1a2, 0xf1, 0xfe},
    {0x11b, 0x165, 0x116, 0x123},
}};
constexpr std::array<MenuRectangle, 3> kNetworkModeRectangles{{
    {0xf0, 0x18f, 0xcf, 0xdc},
    {0xee, 0x191, 0xf1, 0xfe},
    {0x11b, 0x165, 0x116, 0x123},
}};
constexpr MenuRectangle kHostConnectRectangle{
    0x171, 0x196, 0x114, 0x120};
constexpr MenuRectangle kHostBackRectangle{
    0xe9, 0x132, 0x114, 0x120};
constexpr MenuRectangle kHostPasteRectangle{
    0x175, 0x189, 0xe3, 0xf5};

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

void updateTextEntry(
    std::string& text,
    const CharacterSelectFrameInput& input) {
    if (input.backspace_pressed) {
        eraseLastTextCharacter(text);
    }
    appendTextLimited(text, input.text_input, 15);
}

void updateThreeChoiceScreen(
    CharacterSelectStateData& data,
    const CharacterSelectFrameInput& input,
    CharacterSelectFrameResult& result,
    const std::array<MenuRectangle, 3>& rectangles,
    bool network_screen) {
    const auto returnToParent = [&data, network_screen] {
        data.dialog_selection = 0;
        if (network_screen) {
            data.screen = 10;
            return;
        }

        data.brightness_increasing = 1;
        if (data.mode == CharacterSelectMode::new_character) {
            data.name_entry_active = true;
            data.screen = 1;
        } else {
            data.screen = 0;
        }
    };

    const std::int32_t previous = data.dialog_selection;
    bool pointerConfirm = false;
    if (data.input_latch == 0) {
        if ((input.up_pressed || input.left_pressed) &&
            data.dialog_selection > 0) {
            --data.dialog_selection;
        }
        if ((input.down_pressed || input.right_pressed) &&
            data.dialog_selection < 2) {
            ++data.dialog_selection;
        }
    }
    for (std::int32_t item = 0; item < 3; ++item) {
        if (isInside(
                rectangles[static_cast<std::size_t>(item)],
                input.pointer_x,
                input.pointer_y)) {
            data.dialog_selection = item;
            if (input.pointer_primary_pressed &&
                data.input_latch == 0) {
                pointerConfirm = true;
            }
        }
    }
    if (data.dialog_selection != previous) {
        ++result.play_move_sound_count;
    }

    const bool confirm =
        data.input_latch == 0 &&
        (input.confirm_pressed || pointerConfirm);
    if (!confirm && !(input.back_pressed && data.input_latch == 0)) {
        return;
    }
    if (input.back_pressed) {
        returnToParent();
    } else if (!network_screen) {
        if (data.dialog_selection == 0) {
            data.dialog_selection = 0;
            data.screen = 11;
        } else if (data.dialog_selection == 1) {
            data.selection_result = 0;
            data.screen = 20;
        } else {
            returnToParent();
        }
    } else {
        if (data.dialog_selection == 0) {
            data.selection_result = 1;
            data.screen = 20;
        } else if (data.dialog_selection == 1) {
            data.selection_result = 2;
            data.host_entry_active = true;
            data.dialog_selection = 0;
            data.screen = 12;
        } else {
            data.dialog_selection = 0;
            data.screen = 10;
        }
    }
    data.input_latch = 1;
    ++result.play_selection_sound_count;
}

void updateHostScreenImpl(
    CharacterSelectStateData& data,
    const CharacterSelectFrameInput& input,
    CharacterSelectFrameResult& result,
    const std::function<std::string()>& read_clipboard) {
    updateTextEntry(data.host_address, input);
    if (input.pointer_primary_pressed &&
        isInside(
            kHostPasteRectangle,
            input.pointer_x,
            input.pointer_y) &&
        read_clipboard) {
        appendTextLimited(
            data.host_address,
            read_clipboard(),
            15);
    }
    const bool connect =
        (input.confirm_pressed ||
         (input.pointer_primary_pressed &&
          isInside(
              kHostConnectRectangle,
              input.pointer_x,
              input.pointer_y))) &&
        !data.host_address.empty();
    const bool back =
        input.back_pressed ||
        (input.pointer_primary_pressed &&
         isInside(
             kHostBackRectangle,
             input.pointer_x,
             input.pointer_y));
    if (connect) {
        data.selection_result = 2;
        data.host_entry_active = false;
        data.screen = 20;
        data.input_latch = 1;
        ++result.play_selection_sound_count;
    } else if (back) {
        data.host_entry_active = false;
        data.dialog_selection = 1;
        data.screen = 11;
        data.input_latch = 1;
        ++result.play_selection_sound_count;
    }
}

}  // namespace

void updateGameModeScreen(
    CharacterSelectStateData& data,
    const CharacterSelectFrameInput& input,
    CharacterSelectFrameResult& result) {
    updateThreeChoiceScreen(
        data, input, result, kCharacterModeRectangles, false);
}

void updateNetworkModeScreen(
    CharacterSelectStateData& data,
    const CharacterSelectFrameInput& input,
    CharacterSelectFrameResult& result) {
    updateThreeChoiceScreen(
        data, input, result, kNetworkModeRectangles, true);
}

void updateHostScreen(
    CharacterSelectStateData& data,
    const CharacterSelectFrameInput& input,
    CharacterSelectFrameResult& result,
    const std::function<std::string()>& read_clipboard) {
    updateHostScreenImpl(data, input, result, read_clipboard);
}

}  // namespace osf::character_select

