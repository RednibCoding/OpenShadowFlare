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

WorldPosition directionVector(std::int32_t direction) {
    // These are the controller's retail values, not the eight-way CAF
    // direction numbers used for drawing actors.
    switch (direction) {
    case 1:
        return {0, -1};
    case 2:
        return {0, 1};
    case 3:
        return {-1, 0};
    case 4:
        return {1, 0};
    default:
        return {};
    }
}

std::int32_t oppositeDirection(std::int32_t direction) {
    switch (direction) {
    case 1:
        return 2;
    case 2:
        return 1;
    case 3:
        return 4;
    case 4:
        return 3;
    default:
        return 0;
    }
}

bool canMoveOneUnit(
    const GroundMap& ground,
    const ObjectMap& objects,
    const ObjectBounds& bounds,
    WorldPosition position,
    std::int32_t direction) {
    const WorldPosition vector = directionVector(direction);
    return direction != 0 &&
           positionIsWalkable(
               ground,
               objects,
               {
                   position.x + vector.x,
                   position.y + vector.y,
               },
               bounds);
}

bool canMoveDistance(
    const GroundMap& ground,
    const ObjectMap& objects,
    const ObjectBounds& bounds,
    WorldPosition position,
    std::int32_t direction,
    std::int32_t distance) {
    const WorldPosition vector = directionVector(direction);
    bool reached = false;
    furthestWalkablePosition(
        ground,
        objects,
        bounds,
        position,
        {
            position.x + vector.x * std::max(distance, 1),
            position.y + vector.y * std::max(distance, 1),
        },
        reached);
    return direction != 0 && reached;
}

struct ObstacleTurn {
    std::int32_t movement = 0;
    std::int32_t wall = 0;
};

ObstacleTurn chooseObstacleTurn(
    const GroundMap& ground,
    const ObjectMap& objects,
    const ObjectBounds& bounds,
    WorldPosition position,
    WorldPosition destination) {
    const std::int32_t horizontal =
        destination.x == position.x
            ? 0
            : (destination.x > position.x ? 4 : 3);
    const std::int32_t vertical =
        destination.y == position.y
            ? 0
            : (destination.y > position.y ? 2 : 1);
    ObstacleTurn candidates[4]{};
    if (horizontal != 0 && vertical != 0) {
        candidates[0] = {horizontal, vertical};
        candidates[1] = {vertical, horizontal};
        candidates[2] = {
            oppositeDirection(horizontal), vertical};
        candidates[3] = {
            oppositeDirection(vertical), horizontal};
    } else if (horizontal != 0) {
        candidates[0] = {2, horizontal};
        candidates[1] = {1, horizontal};
    } else if (vertical != 0) {
        candidates[0] = {4, vertical};
        candidates[1] = {3, vertical};
    }
    for (const ObstacleTurn& candidate : candidates) {
        if (canMoveOneUnit(
                ground,
                objects,
                bounds,
                position,
                candidate.movement)) {
            return candidate;
        }
    }
    return {};
}

bool directStepIsWalkable(
    const GroundMap& ground,
    const ObjectMap& objects,
    const ObjectBounds& bounds,
    WorldPosition position,
    WorldPosition destination,
    std::int32_t speed) {
    const WorldPosition candidate =
        movementCandidate(position, destination, speed);
    bool reached = false;
    furthestWalkablePosition(
        ground,
        objects,
        bounds,
        position,
        candidate,
        reached);
    return reached;
}

}  // namespace

void MovementController::reset() {
    obstacle_direction_ = 0;
    wall_direction_ = 0;
    detour_start_distance_ = 0;
}

MovementStepResult MovementController::advance(
    const GroundMap& ground,
    const ObjectMap& objects,
    const ObjectBounds& bounds,
    WorldPosition position,
    WorldPosition destination,
    std::int32_t speed,
    bool stop_if_destination_blocked) {
    if (position.x == destination.x &&
        position.y == destination.y) {
        reset();
        return {position, true, false};
    }
    if (speed <= 0) {
        reset();
        return {position, false, false};
    }

    if (obstacle_direction_ != 0 &&
        directStepIsWalkable(
            ground,
            objects,
            bounds,
            position,
            destination,
            speed) &&
        squaredDistance(position, destination) <
            detour_start_distance_) {
        reset();
        return {position, false, false, true};
    }

    if (obstacle_direction_ == 0) {
        const MovementStepResult direct =
            advanceMovement(
                ground,
                objects,
                bounds,
                position,
                destination,
                speed);
        if (direct.reached_destination) {
            reset();
            return direct;
        }
        if (direct.moved) {
            return {
                direct.position,
                false,
                true,
                true,
            };
        }
        if (stop_if_destination_blocked &&
            !positionIsWalkable(
                ground,
                objects,
                destination,
                bounds)) {
            reset();
            return {position, false, false};
        }

        const ObstacleTurn turn =
            chooseObstacleTurn(
                ground,
                objects,
                bounds,
                position,
                destination);
        obstacle_direction_ = turn.movement;
        wall_direction_ = turn.wall;
        detour_start_distance_ =
            squaredDistance(position, destination);
        if (obstacle_direction_ == 0) {
            return {position, false, false};
        }
    }

    const WorldPosition direction =
        directionVector(obstacle_direction_);
    const MovementStepResult edge_step =
        advanceMovement(
            ground,
            objects,
            bounds,
            position,
            {
                position.x + direction.x * speed,
                position.y + direction.y * speed,
            },
            speed);
    if (!edge_step.moved) {
        const std::int32_t old_movement =
            obstacle_direction_;
        obstacle_direction_ =
            oppositeDirection(wall_direction_);
        wall_direction_ = old_movement;
        return {position, false, false, true};
    }

    if (canMoveDistance(
            ground,
            objects,
            bounds,
            edge_step.position,
            wall_direction_,
            speed)) {
        const std::int32_t old_movement =
            obstacle_direction_;
        obstacle_direction_ = wall_direction_;
        wall_direction_ = oppositeDirection(old_movement);
    }
    return {
        edge_step.position,
        edge_step.reached_destination,
        edge_step.moved,
        true,
    };
}

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
