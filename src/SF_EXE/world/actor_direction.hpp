#ifndef OPENSHADOWFLARE_ACTOR_DIRECTION_HPP
#define OPENSHADOWFLARE_ACTOR_DIRECTION_HPP

#include <cstdint>

namespace osf {

std::int32_t retailDirectionForVector(
    std::int32_t x,
    std::int32_t y);
double retailAngleForVector(
    std::int32_t x,
    std::int32_t y);
double retailAngleForDirection(
    std::int32_t direction);

}  // namespace osf

#endif
