#include "input_adapter.hpp"

#include "gapi/gapi.hpp"

#include <cstring>

namespace osf::runtime {

InputAdapter::InputAdapter(
    std::int32_t virtual_width,
    std::int32_t virtual_height)
    : virtual_width_(virtual_width),
      virtual_height_(virtual_height) {}

bool InputAdapter::handleEvent(
    LwlWindow* window,
    const LwlEvent& event,
    GameState current_state) {
    if (event.type == LWL_EVENT_QUIT) {
        return false;
    }
    if (event.type == LWL_EVENT_RESIZED) {
        return true;
    }
    if (event.type == LWL_EVENT_MOUSE_MOVE) {
        setPointerPosition(window, event.x, event.y);
        return true;
    }
    if (event.type == LWL_EVENT_MOUSE_DOWN && event.button == 1) {
        setPointerPosition(window, event.x, event.y);
        menu_.pointer_primary_pressed = true;
        character_select_.pointer_primary_pressed = true;
        pointer_primary_down_ = true;
        return true;
    }
    if (event.type == LWL_EVENT_MOUSE_UP && event.button == 1) {
        setPointerPosition(window, event.x, event.y);
        pointer_primary_down_ = false;
        return true;
    }
    if (event.type == LWL_EVENT_MOUSE_DOWN && event.button == 3) {
        setPointerPosition(window, event.x, event.y);
        pointer_secondary_pressed_ = true;
        return true;
    }
    if (event.type == LWL_EVENT_TEXT_INPUT) {
        character_select_.text_input += event.text;
        return true;
    }
    if (event.type == LWL_EVENT_KEY_UP) {
        if (std::strcmp(event.key, "up") == 0) {
            up_held_ = false;
        } else if (std::strcmp(event.key, "down") == 0) {
            down_held_ = false;
        } else if (std::strcmp(event.key, "left") == 0) {
            left_held_ = false;
        } else if (std::strcmp(event.key, "right") == 0) {
            right_held_ = false;
        } else if (
            std::strcmp(event.key, "return") == 0 ||
            std::strcmp(event.key, "keypad enter") == 0) {
            confirm_held_ = false;
        } else if (std::strcmp(event.key, "escape") == 0) {
            back_held_ = false;
        } else if (std::strcmp(event.key, "delete") == 0) {
            delete_held_ = false;
        } else if (std::strcmp(event.key, "backspace") == 0) {
            backspace_held_ = false;
        } else if (std::strcmp(event.key, "r") == 0) {
            run_held_ = false;
        } else if (std::strcmp(event.key, "h") == 0) {
            help_held_ = false;
        } else if (std::strcmp(event.key, "q") == 0) {
            mission_list_held_ = false;
        } else if (std::strcmp(event.key, "n") == 0) {
            map_held_ = false;
        } else if (std::strcmp(event.key, "i") == 0) {
            inventory_held_ = false;
        }
        return true;
    }
    if (event.type != LWL_EVENT_KEY_DOWN) {
        return true;
    }

    if (std::strcmp(event.key, "escape") == 0) {
        if (current_state == GameState::character_select) {
            if (!back_held_) {
                character_select_.back_pressed = true;
            }
            back_held_ = true;
            return true;
        }
        if (current_state == GameState::gameplay) {
            if (!back_held_) {
                gameplay_options_pressed_ = true;
            }
            back_held_ = true;
            return true;
        }
        return false;
    }
    if (std::strcmp(event.key, "up") == 0) {
        if (!up_held_) {
            menu_.up_pressed = true;
            character_select_.up_pressed = true;
        }
        up_held_ = true;
    } else if (std::strcmp(event.key, "down") == 0) {
        if (!down_held_) {
            menu_.down_pressed = true;
            character_select_.down_pressed = true;
        }
        down_held_ = true;
    } else if (std::strcmp(event.key, "left") == 0) {
        if (!left_held_) {
            character_select_.left_pressed = true;
        }
        left_held_ = true;
    } else if (std::strcmp(event.key, "right") == 0) {
        if (!right_held_) {
            character_select_.right_pressed = true;
        }
        right_held_ = true;
    } else if (
        std::strcmp(event.key, "return") == 0 ||
        std::strcmp(event.key, "keypad enter") == 0) {
        if (!confirm_held_) {
            menu_.confirm_pressed = true;
            character_select_.confirm_pressed = true;
        }
        confirm_held_ = true;
    } else if (std::strcmp(event.key, "delete") == 0) {
        if (!delete_held_) {
            character_select_.delete_pressed = true;
        }
        delete_held_ = true;
    } else if (std::strcmp(event.key, "backspace") == 0) {
        if (!backspace_held_) {
            character_select_.backspace_pressed = true;
        }
        backspace_held_ = true;
    } else if (std::strcmp(event.key, "r") == 0) {
        if (!run_held_) {
            run_toggle_pressed_ = true;
        }
        run_held_ = true;
    } else if (
        std::strcmp(event.key, "h") == 0 &&
        current_state == GameState::gameplay) {
        if (!help_held_) {
            gameplay_help_pressed_ = true;
        }
        help_held_ = true;
    } else if (
        std::strcmp(event.key, "q") == 0 &&
        current_state == GameState::gameplay) {
        if (!mission_list_held_) {
            gameplay_mission_list_pressed_ = true;
        }
        mission_list_held_ = true;
    } else if (
        std::strcmp(event.key, "n") == 0 &&
        current_state == GameState::gameplay) {
        if (!map_held_) {
            gameplay_map_pressed_ = true;
        }
        map_held_ = true;
    } else if (
        std::strcmp(event.key, "i") == 0 &&
        current_state == GameState::gameplay) {
        if (!inventory_held_) {
            gameplay_inventory_pressed_ = true;
        }
        inventory_held_ = true;
    }
    return true;
}

void InputAdapter::clearTransientInput() {
    menu_.pointer_primary_pressed = false;
    menu_.confirm_pressed = false;
    menu_.up_pressed = false;
    menu_.down_pressed = false;
    character_select_.pointer_primary_pressed = false;
    character_select_.confirm_pressed = false;
    character_select_.back_pressed = false;
    character_select_.delete_pressed = false;
    character_select_.up_pressed = false;
    character_select_.down_pressed = false;
    character_select_.left_pressed = false;
    character_select_.right_pressed = false;
    character_select_.backspace_pressed = false;
    character_select_.text_input.clear();
    run_toggle_pressed_ = false;
    gameplay_options_pressed_ = false;
    gameplay_help_pressed_ = false;
    gameplay_mission_list_pressed_ = false;
    gameplay_map_pressed_ = false;
    gameplay_inventory_pressed_ = false;
    pointer_secondary_pressed_ = false;
}

MenuFrameInput& InputAdapter::menu() {
    return menu_;
}

const MenuFrameInput& InputAdapter::menu() const {
    return menu_;
}

CharacterSelectFrameInput&
InputAdapter::characterSelect() {
    return character_select_;
}

const CharacterSelectFrameInput&
InputAdapter::characterSelect() const {
    return character_select_;
}

bool InputAdapter::pointerPrimaryDown() const {
    return pointer_primary_down_;
}

bool InputAdapter::pointerSecondaryPressed() const {
    return pointer_secondary_pressed_;
}

bool InputAdapter::runTogglePressed() const {
    return run_toggle_pressed_;
}

bool InputAdapter::gameplayOptionsPressed() const {
    return gameplay_options_pressed_;
}

bool InputAdapter::gameplayHelpPressed() const {
    return gameplay_help_pressed_;
}

bool InputAdapter::gameplayMissionListPressed() const {
    return gameplay_mission_list_pressed_;
}

bool InputAdapter::gameplayMapPressed() const {
    return gameplay_map_pressed_;
}

bool InputAdapter::gameplayInventoryPressed() const {
    return gameplay_inventory_pressed_;
}

bool InputAdapter::upHeld() const {
    return up_held_;
}

bool InputAdapter::downHeld() const {
    return down_held_;
}

bool InputAdapter::leftHeld() const {
    return left_held_;
}

bool InputAdapter::rightHeld() const {
    return right_held_;
}

void InputAdapter::setPointerPosition(
    LwlWindow* window,
    std::int32_t x,
    std::int32_t y) {
    int width = virtual_width_;
    int height = virtual_height_;
    lwl_window_get_size(window, &width, &height);
    if (width <= 0 || height <= 0) {
        return;
    }
    const gapi::Viewport viewport =
        gapi::fitViewport(
            virtual_width_,
            virtual_height_,
            width,
            height);
    menu_.pointer_x =
        (x - viewport.x) * virtual_width_ / viewport.width;
    menu_.pointer_y =
        (y - viewport.y) * virtual_height_ / viewport.height;
    character_select_.pointer_x = menu_.pointer_x;
    character_select_.pointer_y = menu_.pointer_y;
}

}  // namespace osf::runtime
