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
    pointer_ground_command_active_ = false;
    continuous_pointer_movement_ = false;
    previous_pointer_down_ = false;
    pointer_consumed_until_release_ = false;
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
    pointer_ground_command_active_ = false;
    continuous_pointer_movement_ = false;
    previous_pointer_down_ = false;
    pointer_consumed_until_release_ = false;
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
            pointer_ground_command_active_ = false;
            continuous_pointer_movement_ = false;
            if (input.pointer_primary_pressed ||
                input.pointer_primary_down) {
                pointer_consumed_until_release_ = true;
            } else {
                pointer_consumed_until_release_ = false;
            }
            const bool selection_required =
                hooks_.conversation_requires_selection &&
                hooks_.conversation_requires_selection();
            if (selection_required) {
                bool option_selected = false;
                if (input.pointer_primary_pressed &&
                    hooks_.choose_conversation_option) {
                    option_selected =
                        hooks_.choose_conversation_option(
                            input.pointer_x, input.pointer_y);
                }
                if (!option_selected &&
                    input.confirm_pressed &&
                    hooks_.advance_conversation) {
                    hooks_.advance_conversation();
                }
            } else if (
                (input.confirm_pressed ||
                 input.pointer_primary_pressed) &&
                hooks_.advance_conversation) {
                hooks_.advance_conversation();
            }
            // Conversation display owns gameplay input until its current
            // message has been acknowledged.
        } else {
            const bool pointer_consumed =
                pointer_consumed_until_release_;
            if (pointer_consumed &&
                !input.pointer_primary_down) {
                pointer_consumed_until_release_ = false;
            }
            if (input.run_toggle_pressed &&
                hooks_.toggle_player_run) {
                hooks_.toggle_player_run();
            }
            bool interaction_handled = false;
            if (!pointer_consumed &&
                input.pointer_primary_pressed &&
                input.pointer_x >= 0 &&
                input.pointer_x < 640 &&
                input.pointer_y >= 0 &&
                input.pointer_y < 480 &&
                hooks_.command_world_interaction) {
                interaction_handled =
                    hooks_.command_world_interaction(
                        input.pointer_x, input.pointer_y);
            }
            if (interaction_handled) {
                pointer_ground_command_active_ = false;
                continuous_pointer_movement_ = false;
            }
            if (!pointer_consumed &&
                input.pointer_primary_down &&
                !input.pointer_primary_pressed &&
                pointer_ground_command_active_) {
                continuous_pointer_movement_ = true;
            }
            if (!pointer_consumed &&
                (input.pointer_primary_pressed ||
                 input.pointer_primary_down) &&
                !interaction_handled &&
                (!hooks_.world_interaction_pending ||
                 !hooks_.world_interaction_pending()) &&
                input.pointer_x >= 0 &&
                input.pointer_x < 640 &&
                input.pointer_y >= 0 &&
                input.pointer_y < 480 &&
                hooks_.command_player_movement) {
                hooks_.command_player_movement(
                    input.pointer_x, input.pointer_y);
                if (input.pointer_primary_pressed) {
                    pointer_ground_command_active_ = true;
                    continuous_pointer_movement_ = false;
                }
            }
            if (previous_pointer_down_ &&
                !input.pointer_primary_down) {
                if (pointer_ground_command_active_ &&
                    continuous_pointer_movement_ &&
                    hooks_.cancel_player_movement) {
                    hooks_.cancel_player_movement();
                }
                pointer_ground_command_active_ = false;
                continuous_pointer_movement_ = false;
            } else if (
                input.pointer_primary_pressed &&
                !input.pointer_primary_down) {
                pointer_ground_command_active_ = false;
            }
        }
        if (hooks_.update_world) {
            hooks_.update_world();
        }
        previous_pointer_down_ =
            input.pointer_primary_down;
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
