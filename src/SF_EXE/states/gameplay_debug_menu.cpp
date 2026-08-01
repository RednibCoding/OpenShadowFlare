#include "gameplay_debug_menu.hpp"

namespace osf {
namespace {

bool inside(
    const GameplayDebugInput& input,
    std::int32_t left,
    std::int32_t top,
    std::int32_t right,
    std::int32_t bottom) {
    return input.pointer_x >= left &&
           input.pointer_x < right &&
           input.pointer_y >= top &&
           input.pointer_y < bottom;
}

bool setBooleanRow(
    const GameplayDebugInput& input,
    std::int32_t top,
    bool& value) {
    if (!input.pointer_primary_pressed ||
        input.pointer_y < top ||
        input.pointer_y >= top + 12) {
        return false;
    }
    if (input.pointer_x >= 376 &&
        input.pointer_x < 426) {
        const bool changed = !value;
        value = true;
        return changed;
    }
    if (input.pointer_x >= 426 &&
        input.pointer_x < 464) {
        const bool changed = value;
        value = false;
        return changed;
    }
    return false;
}

}  // namespace

GameplayDebugResult GameplayDebugMenu::update(
    const GameplayDebugInput& input) {
    pointer_x_ = input.pointer_x;
    pointer_y_ = input.pointer_y;

    GameplayDebugResult result;
    if (input.toggle_pressed) {
        active_ = !active_;
        result.play_confirm_sound = true;
        return result;
    }
    if (!active_) {
        return result;
    }
    if (input.close_pressed) {
        close();
        result.play_confirm_sound = true;
        return result;
    }
    if (input.pointer_primary_pressed &&
        inside(input, 176, 198, 464, 210)) {
        close();
        result.play_confirm_sound = true;
        return result;
    }

    result.settings_changed =
        setBooleanRow(input, 118, fps_counter_enabled_) ||
        setBooleanRow(input, 134, all_spells_enabled_) ||
        setBooleanRow(input, 150, infinite_life_enabled_) ||
        setBooleanRow(input, 166, infinite_mana_enabled_);
    result.play_click_sound = result.settings_changed;
    return result;
}

void GameplayDebugMenu::close() {
    active_ = false;
}

bool GameplayDebugMenu::active() const {
    return active_;
}

bool GameplayDebugMenu::fpsCounterEnabled() const {
    return fps_counter_enabled_;
}

bool GameplayDebugMenu::allSpellsEnabled() const {
    return all_spells_enabled_;
}

bool GameplayDebugMenu::infiniteLifeEnabled() const {
    return infinite_life_enabled_;
}

bool GameplayDebugMenu::infiniteManaEnabled() const {
    return infinite_mana_enabled_;
}

std::int32_t GameplayDebugMenu::pointerX() const {
    return pointer_x_;
}

std::int32_t GameplayDebugMenu::pointerY() const {
    return pointer_y_;
}

}  // namespace osf
