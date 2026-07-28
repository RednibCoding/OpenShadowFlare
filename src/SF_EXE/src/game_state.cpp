#include "openshadowflare/game_state.hpp"

#include <utility>

namespace openshadowflare {
namespace {

constexpr std::int32_t retailValue(GameState state) {
    return static_cast<std::int32_t>(state);
}

void leaveState(
    std::int32_t state,
    const GameStateDispatcherCallbacks& callbacks) {
    const std::function<void()>* leave = nullptr;
    if (state == retailValue(GameState::title)) {
        leave = &callbacks.title.leave;
    } else if (state == retailValue(GameState::loading)) {
        leave = &callbacks.loading.leave;
    } else if (state == retailValue(GameState::gameplay)) {
        leave = &callbacks.gameplay.leave;
    }
    if (leave != nullptr && *leave) {
        (*leave)();
    }
}

void enterState(
    std::int32_t state,
    std::int32_t argument,
    const GameStateDispatcherCallbacks& callbacks) {
    const std::function<void(std::int32_t)>* enter = nullptr;
    std::int32_t forwarded_argument = 0;
    if (state == retailValue(GameState::title)) {
        enter = &callbacks.title.enter;
    } else if (state == retailValue(GameState::loading)) {
        enter = &callbacks.loading.enter;
        forwarded_argument = argument;
    } else if (state == retailValue(GameState::gameplay)) {
        enter = &callbacks.gameplay.enter;
    }
    if (enter != nullptr && *enter) {
        (*enter)(forwarded_argument);
    }
}

}  // namespace

GameStateDispatcher::GameStateDispatcher(
    GameStateDispatcherCallbacks callbacks)
    : callbacks_(std::move(callbacks)) {}

void GameStateDispatcher::transition(
    GameState state,
    std::int32_t argument) {
    transition(retailValue(state), argument);
}

void GameStateDispatcher::transition(
    std::int32_t retail_state,
    std::int32_t argument) {
    if (callbacks_.wait_until_renderer_idle) {
        callbacks_.wait_until_renderer_idle();
    }
    leaveState(current_state_, callbacks_);
    enterState(retail_state, argument, callbacks_);

    // The original also stores unrecognized values after leaving the current
    // recognized state, so this deliberately is not restricted to the enum.
    current_state_ = retail_state;
}

std::int32_t GameStateDispatcher::currentRetailState() const {
    return current_state_;
}

GameState GameStateDispatcher::currentState() const {
    return static_cast<GameState>(current_state_);
}

}  // namespace openshadowflare
