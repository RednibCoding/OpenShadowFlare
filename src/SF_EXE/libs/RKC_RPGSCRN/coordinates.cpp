#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <cstdint>

namespace osf {

ScreenPosition calculateRealPosition(
    WorldPosition position,
    std::int32_t base_x,
    std::int32_t base_y) {
    return {
        static_cast<std::int32_t>(
            (static_cast<std::int64_t>(
                 position.x - position.y) *
             base_x) /
            100),
        static_cast<std::int32_t>(
            (static_cast<std::int64_t>(
                 position.x + position.y) *
             base_y) /
            100),
    };
}

WorldPosition calculateWorldPosition(
    ScreenPosition position,
    std::int32_t base_x,
    std::int32_t base_y) {
    if (base_x == 0 || base_y == 0) {
        return {};
    }
    const std::int64_t divisor =
        static_cast<std::int64_t>(base_x) * base_y * 2;
    const std::int64_t numerator_x =
        static_cast<std::int64_t>(base_x) * position.y +
        static_cast<std::int64_t>(base_y) * position.x;
    const std::int64_t numerator_y =
        static_cast<std::int64_t>(base_x) * position.y -
        static_cast<std::int64_t>(base_y) * position.x;
    WorldPosition result{
        static_cast<std::int32_t>(
            numerator_x * 100 / divisor),
        static_cast<std::int32_t>(
            numerator_y * 100 / divisor),
    };
    if (numerator_x < 0) {
        --result.x;
    }
    if (numerator_y < 0) {
        --result.y;
    }
    return result;
}

}  // namespace osf
