#include "movement_controller.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace osf {
namespace {

std::int32_t separatedEdgeDistance(
    std::int32_t first_minimum,
    std::int32_t first_maximum,
    std::int32_t second_minimum,
    std::int32_t second_maximum) {
    if (first_maximum < second_minimum) {
        return second_minimum - first_maximum;
    }
    if (second_maximum < first_minimum) {
        return first_minimum - second_maximum;
    }
    return 0;
}

WorldPosition movementCandidate(
    WorldPosition position,
    WorldPosition destination,
    std::int32_t speed) {
    const std::int64_t delta_x =
        static_cast<std::int64_t>(destination.x) - position.x;
    const std::int64_t delta_y =
        static_cast<std::int64_t>(destination.y) - position.y;
    const double distance = std::hypot(
        static_cast<double>(delta_x),
        static_cast<double>(delta_y));
    if (distance <= static_cast<double>(speed) || distance < 1.0) {
        return destination;
    }
    return {
        position.x + static_cast<std::int32_t>(
                         static_cast<double>(delta_x) /
                         distance * speed),
        position.y + static_cast<std::int32_t>(
                         static_cast<double>(delta_y) /
                         distance * speed),
    };
}

WorldPosition furthestWalkablePosition(
    const GroundMap& ground,
    const ObjectMap& objects,
    const ObjectBounds& bounds,
    WorldPosition start,
    WorldPosition end,
    bool& reached) {
    const std::int32_t delta_x = end.x - start.x;
    const std::int32_t delta_y = end.y - start.y;
    const std::int32_t steps = std::max(
        std::abs(delta_x), std::abs(delta_y));
    WorldPosition result = start;
    reached = true;
    for (std::int32_t step = 1; step <= steps; ++step) {
        const WorldPosition position{
            start.x + delta_x * step / steps,
            start.y + delta_y * step / steps,
        };
        if (!positionIsWalkable(
                ground, objects, position, bounds)) {
            reached = false;
            break;
        }
        result = position;
    }
    return result;
}

std::int64_t squaredDistance(
    WorldPosition first,
    WorldPosition second) {
    const std::int64_t x =
        static_cast<std::int64_t>(first.x) - second.x;
    const std::int64_t y =
        static_cast<std::int64_t>(first.y) - second.y;
    return x * x + y * y;
}

}  // namespace

std::int32_t distanceBetweenBounds(
    WorldPosition first_position,
    const ObjectBounds& first_bounds,
    WorldPosition second_position,
    const ObjectBounds& second_bounds) {
    const std::int32_t horizontal =
        separatedEdgeDistance(
            first_position.x + first_bounds.left,
            first_position.x + first_bounds.right,
            second_position.x + second_bounds.left,
            second_position.x + second_bounds.right);
    const std::int32_t vertical =
        separatedEdgeDistance(
            first_position.y + first_bounds.top,
            first_position.y + first_bounds.bottom,
            second_position.y + second_bounds.top,
            second_position.y + second_bounds.bottom);
    if (horizontal == 0 && vertical == 0) {
        return 0;
    }
    if (horizontal == 0 || vertical == 0) {
        return std::max(horizontal + vertical - 1, 0);
    }
    return std::max(
        static_cast<std::int32_t>(
            std::hypot(
                static_cast<double>(horizontal),
                static_cast<double>(vertical))) -
            1,
        0);
}

MovementStepResult advanceMovement(
    const GroundMap& ground,
    const ObjectMap& objects,
    const ObjectBounds& bounds,
    WorldPosition position,
    WorldPosition destination,
    std::int32_t speed) {
    if (position.x == destination.x &&
        position.y == destination.y) {
        return {position, true, false};
    }

    const WorldPosition candidate =
        movementCandidate(
            position, destination, std::max(speed, 0));
    bool reached = false;
    const WorldPosition direct =
        furthestWalkablePosition(
            ground,
            objects,
            bounds,
            position,
            candidate,
            reached);
    if (reached) {
        return {
            direct,
            direct.x == destination.x &&
                direct.y == destination.y,
            direct.x != position.x || direct.y != position.y,
        };
    }

    bool ignored = false;
    const WorldPosition x_slide =
        furthestWalkablePosition(
            ground,
            objects,
            bounds,
            direct,
            {candidate.x, direct.y},
            ignored);
    const WorldPosition y_slide =
        furthestWalkablePosition(
            ground,
            objects,
            bounds,
            direct,
            {direct.x, candidate.y},
            ignored);
    const WorldPosition result =
        squaredDistance(position, x_slide) >=
                squaredDistance(position, y_slide)
            ? x_slide
            : y_slide;
    return {
        result,
        result.x == destination.x &&
            result.y == destination.y,
        result.x != position.x || result.y != position.y,
    };
}

WorldPosition interpolateWorldPosition(
    WorldPosition previous,
    WorldPosition current,
    double alpha) {
    const double amount = std::clamp(alpha, 0.0, 1.0);
    return {
        previous.x + static_cast<std::int32_t>(
                         std::lround(
                             static_cast<double>(
                                 current.x - previous.x) *
                             amount)),
        previous.y + static_cast<std::int32_t>(
                         std::lround(
                             static_cast<double>(
                                 current.y - previous.y) *
                             amount)),
    };
}

}  // namespace osf
