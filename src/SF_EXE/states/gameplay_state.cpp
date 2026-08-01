#include "gameplay_state.hpp"

#include <utility>

namespace osf {
namespace {

bool pointerInsideWorldView(const GameplayFrameInput& input) {
    return input.pointer_x >= input.world_view_left &&
           input.pointer_x < input.world_view_right &&
           input.pointer_y >= input.world_view_top &&
           input.pointer_y < input.world_view_bottom;
}

}  // namespace

GameplayState::GameplayState(GameplayStateHooks hooks)
    : hooks_(std::move(hooks)) {}

void GameplayState::enter() {
    phase_ = GameplayPhase::loading;
    loading_counter_ = 0;
    interface_ready_ =
        !hooks_.prepare_interface ||
        hooks_.prepare_interface();
    world_ready_ =
        interface_ready_ &&
        (!hooks_.prepare_world || hooks_.prepare_world());
    if (world_ready_ && hooks_.start_world_music) {
        hooks_.start_world_music();
    }
    active_ = true;
    pointer_ground_command_active_ = false;
    continuous_pointer_movement_ = false;
    pointer_hold_updates_ = 0;
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
    if (interface_ready_ && hooks_.release_interface) {
        hooks_.release_interface();
    }
    active_ = false;
    world_ready_ = false;
    interface_ready_ = false;
    pointer_ground_command_active_ = false;
    continuous_pointer_movement_ = false;
    pointer_hold_updates_ = 0;
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
        const bool pointer_in_world =
            pointerInsideWorldView(input);
        if (pointer_in_world &&
            hooks_.update_pointer_hover) {
            hooks_.update_pointer_hover(
                input.pointer_x, input.pointer_y);
        } else if (
            !pointer_in_world &&
            hooks_.clear_pointer_hover) {
            hooks_.clear_pointer_hover();
        }
        const bool conversation_active =
            hooks_.conversation_active &&
            hooks_.conversation_active();
        if (conversation_active) {
            pointer_ground_command_active_ = false;
            continuous_pointer_movement_ = false;
            pointer_hold_updates_ = 0;
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
            if (input.increased_power_pressed &&
                hooks_.activate_increased_power) {
                hooks_.activate_increased_power();
            }
            if (input.pointer_secondary_pressed &&
                pointer_in_world &&
                hooks_.command_player_magic) {
                hooks_.command_player_magic(
                    input.pointer_x,
                    input.pointer_y);
            }
            bool interaction_handled = false;
            if (!pointer_consumed &&
                input.pointer_primary_pressed &&
                pointer_in_world &&
                hooks_.command_world_interaction) {
                interaction_handled =
                    hooks_.command_world_interaction(
                        input.pointer_x, input.pointer_y);
            }
            if (interaction_handled) {
                pointer_ground_command_active_ = false;
                continuous_pointer_movement_ = false;
                pointer_hold_updates_ = 0;
            }
            if (!pointer_consumed &&
                input.pointer_primary_down &&
                pointer_ground_command_active_) {
                if (input.pointer_primary_pressed) {
                    pointer_hold_updates_ = 1;
                } else {
                    ++pointer_hold_updates_;
                }
                // The retail input record increments its hold counter while
                // the button is down. Release stops movement only after that
                // counter has passed nine updates.
                continuous_pointer_movement_ =
                    pointer_hold_updates_ > 9;
            }
            if (!pointer_consumed &&
                (input.pointer_primary_pressed ||
                 input.pointer_primary_down) &&
                !interaction_handled &&
                (!hooks_.world_interaction_pending ||
                 !hooks_.world_interaction_pending()) &&
                pointer_in_world &&
                hooks_.command_player_movement) {
                hooks_.command_player_movement(
                    input.pointer_x, input.pointer_y);
                if (input.pointer_primary_pressed) {
                    pointer_ground_command_active_ = true;
                    continuous_pointer_movement_ = false;
                    pointer_hold_updates_ =
                        input.pointer_primary_down ? 1 : 0;
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
                pointer_hold_updates_ = 0;
            } else if (
                input.pointer_primary_pressed &&
                !input.pointer_primary_down) {
                pointer_ground_command_active_ = false;
                pointer_hold_updates_ = 0;
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
