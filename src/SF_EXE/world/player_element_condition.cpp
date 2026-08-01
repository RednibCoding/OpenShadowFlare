#include "player_element_condition.hpp"

#include <cstddef>
#include <cmath>

namespace osf {

const std::array<ElementAnchor, 8>& retailElementAnchors() {
    static constexpr std::array<ElementAnchor, 8> anchors{{
        {0, 20000},
        {0, -20000},
        {-20000, 0},
        {20000, 0},
        {14140, -14140},
        {-14140, 14140},
        {-14140, -14140},
        {14140, 14140},
    }};
    return anchors;
}

ElementAnchor moveRetailElementCondition(
    ElementAnchor current,
    std::int32_t element,
    std::int32_t distance) {
    if (element < 0 || element >= 8 || distance <= 0) {
        return current;
    }
    const ElementAnchor target = retailElementAnchors()[
        static_cast<std::size_t>(element)];
    const double delta_x =
        static_cast<double>(target.x) - current.x;
    const double delta_y =
        static_cast<double>(target.y) - current.y;
    if (std::trunc(std::hypot(delta_x, delta_y)) <= distance) {
        return target;
    }
    const double angle = std::atan2(delta_y, delta_x);
    current.x += static_cast<std::int32_t>(
        std::trunc(std::cos(angle) * distance));
    current.y += static_cast<std::int32_t>(
        std::trunc(std::sin(angle) * distance));
    return current;
}

}  // namespace osf
