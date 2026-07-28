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
};

struct GameplayStateHooks {
    std::function<bool()> prepare_world;
    std::function<void()> release_world;
    std::function<void()> start_world_music;
    std::function<void()> stop_world_music;
    std::function<void(std::int32_t, std::int32_t)>
        command_player_movement;
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
};

}  // namespace osf

#endif
