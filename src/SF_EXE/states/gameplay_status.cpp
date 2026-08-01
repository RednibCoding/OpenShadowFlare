#include "gameplay_status.hpp"

namespace osf {
namespace {

bool insideMagicTab(const GameplayStatusInput& input) {
    return input.pointer_x >= 160 &&
           input.pointer_x < 320 &&
           input.pointer_y >= 0 &&
           input.pointer_y < 37;
}

}  // namespace

void GameplayStatus::open() {
    active_ = true;
}

void GameplayStatus::close() {
    active_ = false;
}

GameplayStatusResult GameplayStatus::update(
    const GameplayStatusInput& input) {
    GameplayStatusResult result;
    if (input.toggle_pressed) {
        active_ = !active_;
        result.pointer_consumed = true;
        result.play_move_sound = true;
        return result;
    }
    if (!active_) {
        return result;
    }
    if (input.close_pressed) {
        close();
        result.pointer_consumed = true;
        result.play_move_sound = true;
        return result;
    }
    if (!input.pointer_primary_pressed) {
        return result;
    }

    result.pointer_consumed =
        input.pointer_x < 320 && input.pointer_y < 412;
    if (insideMagicTab(input)) {
        close();
        result.switch_to_magic = true;
        result.play_move_sound = true;
    }
    return result;
}

bool GameplayStatus::active() const {
    return active_;
}

}  // namespace osf
