#ifndef OPENSHADOWFLARE_GAMEPLAY_HUD_INPUT_HPP
#define OPENSHADOWFLARE_GAMEPLAY_HUD_INPUT_HPP

#include <cstdint>

namespace osf {

enum class GameplayHudButton {
    none,
    options,
    status,
    inventory,
};

GameplayHudButton gameplayHudButtonAtPointer(
    bool pointer_pressed,
    std::int32_t pointer_x,
    std::int32_t pointer_y);

}  // namespace osf

#endif
