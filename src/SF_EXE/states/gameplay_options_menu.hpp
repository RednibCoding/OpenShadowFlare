#ifndef OPENSHADOWFLARE_GAMEPLAY_OPTIONS_MENU_HPP
#define OPENSHADOWFLARE_GAMEPLAY_OPTIONS_MENU_HPP

#include "core/game_config.hpp"

#include <cstdint>

namespace osf {

struct GameplayOptionsInput {
    bool toggle_pressed = false;
    bool pointer_primary_pressed = false;
    bool pointer_primary_down = false;
    std::int32_t pointer_x = 0;
    std::int32_t pointer_y = 0;
};

struct GameplayOptionsResult {
    bool config_changed = false;
    bool play_click_sound = false;
};

class GameplayOptionsMenu {
public:
    GameplayOptionsResult update(
        const GameplayOptionsInput& input,
        GameConfig& config);
    void close();

    bool active() const;
    std::int32_t pointerX() const;
    std::int32_t pointerY() const;
    std::int32_t animationCounter() const;

private:
    bool active_ = false;
    std::int32_t pointer_x_ = 0;
    std::int32_t pointer_y_ = 0;
    std::int32_t animation_counter_ = 0;
};

std::int32_t gameplayOptionsVolumeFromPointerX(
    std::int32_t pointer_x);
std::int32_t gameplayOptionsVolumeSliderOffset(
    std::int32_t volume);

}  // namespace osf

#endif
