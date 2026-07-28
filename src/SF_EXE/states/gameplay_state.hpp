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
    std::int32_t animation_frame = 0;
    bool world_ready = false;
    bool ready_to_continue = false;
};

struct GameplayFrameInput {
    bool confirm_pressed = false;
    bool pointer_primary_pressed = false;
    std::int32_t pointer_x = 0;
    std::int32_t pointer_y = 0;
};

struct GameplayStateHooks {
    std::function<bool()> prepare_world;
    std::function<void()> release_world;
    std::function<void()> start_world_music;
    std::function<void()> stop_world_music;
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
    std::int32_t animation_frame_ = 0;
    bool world_ready_ = false;
    bool active_ = false;
};

}  // namespace osf

#endif
