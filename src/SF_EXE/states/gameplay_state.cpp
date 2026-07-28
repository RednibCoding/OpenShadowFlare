#include "gameplay_state.hpp"

#include <utility>

namespace osf {
GameplayState::GameplayState(GameplayStateHooks hooks)
    : hooks_(std::move(hooks)) {}

void GameplayState::enter() {
    phase_ = GameplayPhase::loading;
    loading_counter_ = 0;
    animation_frame_ = 0;
    world_ready_ =
        !hooks_.prepare_world || hooks_.prepare_world();
    if (world_ready_ && hooks_.start_world_music) {
        hooks_.start_world_music();
    }
    active_ = true;
}

void GameplayState::leave() {
    if (active_ && hooks_.stop_world_music) {
        hooks_.stop_world_music();
    }
    if (active_ && hooks_.release_world) {
        hooks_.release_world();
    }
    active_ = false;
    world_ready_ = false;
}

GameplayFrameResult GameplayState::update(
    const GameplayFrameInput& input) {
    if (phase_ == GameplayPhase::loading) {
        ++loading_counter_;
        const bool arrowClicked =
            input.pointer_primary_pressed &&
            input.pointer_x > 591 &&
            input.pointer_x < 628 &&
            input.pointer_y > 449 &&
            input.pointer_y < 472;
        if (world_ready_ &&
            (input.confirm_pressed ||
             arrowClicked)) {
            phase_ = GameplayPhase::world;
        }
    } else {
        ++animation_frame_;
    }
    return {
        phase_,
        loading_counter_,
        animation_frame_,
        world_ready_,
        world_ready_,
    };
}

GameplayPhase GameplayState::phase() const {
    return phase_;
}

bool GameplayState::worldReady() const {
    return world_ready_;
}

}  // namespace osf
