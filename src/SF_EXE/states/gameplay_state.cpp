#include "gameplay_state.hpp"

#include <utility>

namespace osf {
GameplayState::GameplayState(GameplayStateHooks hooks)
    : hooks_(std::move(hooks)) {}

void GameplayState::enter() {
    phase_ = GameplayPhase::loading;
    loading_counter_ = 0;
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
        if (hooks_.update_pointer_hover) {
            hooks_.update_pointer_hover(
                input.pointer_x, input.pointer_y);
        }
        const bool conversation_active =
            hooks_.conversation_active &&
            hooks_.conversation_active();
        if (conversation_active) {
            if ((input.confirm_pressed ||
                 input.pointer_primary_pressed) &&
                hooks_.advance_conversation) {
                hooks_.advance_conversation();
            }
            // Conversation display owns gameplay input until its current
            // message has been acknowledged.
        } else {
            if (input.run_toggle_pressed &&
                hooks_.toggle_player_run) {
                hooks_.toggle_player_run();
            }
            bool interaction_handled = false;
            if (input.pointer_primary_pressed &&
                input.pointer_x >= 0 &&
                input.pointer_x < 640 &&
                input.pointer_y >= 0 &&
                input.pointer_y < 480 &&
                hooks_.command_world_interaction) {
                interaction_handled =
                    hooks_.command_world_interaction(
                        input.pointer_x, input.pointer_y);
            }
            if ((input.pointer_primary_pressed ||
                 input.pointer_primary_down) &&
                !interaction_handled &&
                input.pointer_x >= 0 &&
                input.pointer_x < 640 &&
                input.pointer_y >= 0 &&
                input.pointer_y < 480 &&
                hooks_.command_player_movement) {
                hooks_.command_player_movement(
                    input.pointer_x, input.pointer_y);
            }
        }
        if (hooks_.update_world) {
            hooks_.update_world();
        }
    }
    return {
        phase_,
        loading_counter_,
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
