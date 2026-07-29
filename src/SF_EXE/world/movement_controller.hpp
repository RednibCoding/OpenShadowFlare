#ifndef OPENSHADOWFLARE_MOVEMENT_CONTROLLER_HPP
#define OPENSHADOWFLARE_MOVEMENT_CONTROLLER_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <cstdint>

namespace osf {

struct MovementStepResult {
    WorldPosition position;
    bool reached_destination = false;
    bool moved = false;
    bool controller_active = false;
};

class MovementController {
public:
    void reset();
    MovementStepResult advance(
        const GroundMap& ground,
        const ObjectMap& objects,
        const ObjectBounds& bounds,
        WorldPosition position,
        WorldPosition destination,
        std::int32_t speed,
        bool stop_if_destination_blocked = true);

private:
    std::int32_t obstacle_direction_ = 0;
    std::int32_t wall_direction_ = 0;
    std::int64_t detour_start_distance_ = 0;
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
