#include "companion_hud_input.hpp"

namespace osf {

bool companionHudToggleAtPointer(
    bool pointer_pressed,
    std::int32_t pointer_x,
    std::int32_t pointer_y) {
    // FUN_00445bd0 uses an exclusive 0..112 by 392..409 test. The
    // vertical comparisons are strict, leaving rows 393 through 408.
    return pointer_pressed &&
           pointer_x >= 0 && pointer_x < 112 &&
           pointer_y > 392 && pointer_y < 409;
}

}  // namespace osf
