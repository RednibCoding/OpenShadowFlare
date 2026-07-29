#ifndef OPENSHADOWFLARE_GAMEPLAY_STATE_HPP
#define OPENSHADOWFLARE_GAMEPLAY_STATE_HPP

#include <cstdint>
#include <functional>

namespace osf {

enum class GameplayPhase {
    loading,
    world,
};

struct GameplayFrameResult {
    GameplayPhase phase = GameplayPhase::loading;
    std::int32_t loading_counter = 0;
    bool world_ready = false;
    bool ready_to_continue = false;
};

struct GameplayFrameInput {
    bool confirm_pressed = false;
    bool pointer_primary_pressed = false;
    std::int32_t pointer_x = 0;
    std::int32_t pointer_y = 0;
    bool pointer_primary_down = false;
    bool run_toggle_pressed = false;
    std::int32_t world_view_left = 0;
    std::int32_t world_view_top = 0;
    std::int32_t world_view_right = 640;
    std::int32_t world_view_bottom = 400;
};

struct GameplayStateHooks {
    std::function<bool()> prepare_interface;
    std::function<void()> release_interface;
    std::function<bool()> prepare_world;
    std::function<void()> release_world;
    std::function<void()> start_world_music;
    std::function<void()> stop_world_music;
    std::function<void(std::int32_t, std::int32_t)>
        command_player_movement;
    std::function<void()> cancel_player_movement;
    std::function<void(std::int32_t, std::int32_t)>
        update_pointer_hover;
    std::function<void()> clear_pointer_hover;
    std::function<bool(std::int32_t, std::int32_t)>
        command_world_interaction;
    std::function<bool()> world_interaction_pending;
    std::function<bool()> conversation_active;
    std::function<bool()> conversation_requires_selection;
    std::function<bool(std::int32_t, std::int32_t)>
        choose_conversation_option;
    std::function<void()> advance_conversation;
    std::function<void()> toggle_player_run;
    std::function<void()> update_world;
};

class GameplayState {
public:
    explicit GameplayState(GameplayStateHooks hooks = {});

    void enter();
    void leave();
    GameplayFrameResult update(
        const GameplayFrameInput& input = {});

    GameplayPhase phase() const;
    bool worldReady() const;

private:
    GameplayStateHooks hooks_;
    GameplayPhase phase_ = GameplayPhase::loading;
    std::int32_t loading_counter_ = 0;
    bool world_ready_ = false;
    bool active_ = false;
    bool interface_ready_ = false;
    bool pointer_ground_command_active_ = false;
    bool continuous_pointer_movement_ = false;
    std::int32_t pointer_hold_updates_ = 0;
    bool previous_pointer_down_ = false;
    bool pointer_consumed_until_release_ = false;
};

}  // namespace osf

#endif
