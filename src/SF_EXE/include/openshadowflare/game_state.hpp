#ifndef OPENSHADOWFLARE_GAME_STATE_HPP
#define OPENSHADOWFLARE_GAME_STATE_HPP

#include <cstdint>
#include <functional>

namespace openshadowflare {

enum class GameState : std::int32_t {
    none = -1,
    title = 0,
    loading = 1,
    gameplay = 2,
};

struct GameStateCallbacks {
    std::function<void(std::int32_t)> enter;
    std::function<void()> leave;
};

struct GameStateDispatcherCallbacks {
    // The retail function waits for the renderer before touching state. The
    // portable renderer owns the waiting policy and exposes it through here.
    std::function<void()> wait_until_renderer_idle;
    GameStateCallbacks title;
    GameStateCallbacks loading;
    GameStateCallbacks gameplay;
};

// Portable reconstruction of retail function 0x004023d0.
class GameStateDispatcher {
public:
    explicit GameStateDispatcher(
        GameStateDispatcherCallbacks callbacks = {});

    void transition(GameState state, std::int32_t argument = 0);
    void transition(std::int32_t retail_state, std::int32_t argument = 0);

    std::int32_t currentRetailState() const;
    GameState currentState() const;

private:
    GameStateDispatcherCallbacks callbacks_;
    std::int32_t current_state_ =
        static_cast<std::int32_t>(GameState::none);
};

}  // namespace openshadowflare

#endif
