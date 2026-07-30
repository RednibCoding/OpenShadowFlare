#include "actor_direction.hpp"

#include <cmath>
#include <cstdint>

namespace osf {

std::int32_t retailDirectionForVector(
    std::int32_t x,
    std::int32_t y) {
    double angle = std::atan2(
        -static_cast<double>(y),
        static_cast<double>(x));
    constexpr double full_turn =
        6.28318530717958647692;
    constexpr double radians_to_tenths_of_degrees =
        572.95779513082320877;
    while (angle < 0.0) {
        angle += full_turn;
    }
    while (angle > full_turn) {
        angle -= full_turn;
    }
    const std::int32_t tenths =
        static_cast<std::int32_t>(
            angle * radians_to_tenths_of_degrees) %
        3600;
    if (tenths <= 225 || tenths > 3375) {
        return 1;
    }
    if (tenths <= 675) {
        return 2;
    }
    if (tenths <= 1125) {
        return 3;
    }
    if (tenths <= 1575) {
        return 4;
    }
    if (tenths <= 2025) {
        return 5;
    }
    if (tenths <= 2475) {
        return 6;
    }
    if (tenths <= 2925) {
        return 7;
    }
    return 0;
}

}  // namespace osf
