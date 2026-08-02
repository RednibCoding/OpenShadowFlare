#ifndef OPENSHADOWFLARE_COMPANION_HUD_INPUT_HPP
#define OPENSHADOWFLARE_COMPANION_HUD_INPUT_HPP

#include <cstdint>

namespace osf {

bool companionHudToggleAtPointer(
    bool pointer_pressed,
    std::int32_t pointer_x,
    std::int32_t pointer_y);

}  // namespace osf

#endif
