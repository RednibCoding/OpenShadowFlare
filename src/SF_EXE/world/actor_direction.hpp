#ifndef OPENSHADOWFLARE_ACTOR_DIRECTION_HPP
#define OPENSHADOWFLARE_ACTOR_DIRECTION_HPP

#include <cstdint>

namespace osf {

inline constexpr double kRetailFullCircleRadians = 6.283184;
inline constexpr double kRetailHalfCircleRadians = 3.141592;
inline constexpr double kRetailRadiansPerDegree =
    0.01745328888888889;

std::int32_t retailDirectionForVector(
    std::int32_t x,
    std::int32_t y);
std::int32_t retailDirectionForAngle(double angle);
double retailAngleForVector(
    std::int32_t x,
    std::int32_t y);
double retailAngleForDirection(
    std::int32_t direction);

}  // namespace osf

#endif
