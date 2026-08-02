#include "gameplay_hud_input.hpp"

namespace osf {
namespace {

bool insideInclusive(
    std::int32_t x,
    std::int32_t y,
    std::int32_t left,
    std::int32_t top,
    std::int32_t right,
    std::int32_t bottom) {
    return x >= left && x <= right &&
           y >= top && y <= bottom;
}

}  // namespace

GameplayHudButton gameplayHudButtonAtPointer(
    bool pointer_pressed,
    std::int32_t pointer_x,
    std::int32_t pointer_y) {
    if (!pointer_pressed) {
        return GameplayHudButton::none;
    }
    // HandleBeltAndHudInput (0x00445bd0) uses these authored rectangles
    // rather than deriving hitboxes from the visible text bounds.
    if (insideInclusive(
            pointer_x, pointer_y,
            589, 402, 639, 413)) {
        return GameplayHudButton::options;
    }
    if (insideInclusive(
            pointer_x, pointer_y,
            537, 420, 577, 437)) {
        return GameplayHudButton::status;
    }
    if (insideInclusive(
            pointer_x, pointer_y,
            583, 429, 636, 448)) {
        return GameplayHudButton::inventory;
    }
    return GameplayHudButton::none;
}

}  // namespace osf
