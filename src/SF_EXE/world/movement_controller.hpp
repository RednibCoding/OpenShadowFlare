#ifndef OPENSHADOWFLARE_MOVEMENT_CONTROLLER_HPP
#define OPENSHADOWFLARE_MOVEMENT_CONTROLLER_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <cstdint>

namespace osf {

struct MovementStepResult {
    WorldPosition position;
    bool reached_destination = false;
    bool moved = false;
};

std::int32_t distanceBetweenBounds(
    WorldPosition first_position,
    const ObjectBounds& first_bounds,
    WorldPosition second_position,
    const ObjectBounds& second_bounds);

MovementStepResult advanceMovement(
    const GroundMap& ground,
    const ObjectMap& objects,
    const ObjectBounds& bounds,
    WorldPosition position,
    WorldPosition destination,
    std::int32_t speed);

WorldPosition interpolateWorldPosition(
    WorldPosition previous,
    WorldPosition current,
    double alpha);

}  // namespace osf

#endif
