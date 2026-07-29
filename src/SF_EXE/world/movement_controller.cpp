#include "movement_controller.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace osf {
namespace {

constexpr std::int32_t kNorth = 1;
constexpr std::int32_t kSouth = 2;
constexpr std::int32_t kWest = 3;
constexpr std::int32_t kEast = 4;

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

bool samePosition(
    WorldPosition first,
    WorldPosition second) {
    return first.x == second.x && first.y == second.y;
}

WorldPosition directionVector(std::int32_t direction) {
    switch (direction) {
    case kNorth:
        return {0, -1};
    case kSouth:
        return {0, 1};
    case kWest:
        return {-1, 0};
    case kEast:
        return {1, 0};
    default:
        return {};
    }
}

std::int32_t oppositeDirection(std::int32_t direction) {
    switch (direction) {
    case kNorth:
        return kSouth;
    case kSouth:
        return kNorth;
    case kWest:
        return kEast;
    case kEast:
        return kWest;
    default:
        return 0;
    }
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

bool boundsOverlap(
    WorldPosition first_position,
    const ObjectBounds& first,
    WorldPosition second_position,
    const ObjectBounds& second) {
    return first_position.x + first.left <=
               second_position.x + second.right &&
           second_position.x + second.left <=
               first_position.x + first.right &&
           first_position.y + first.top <=
               second_position.y + second.bottom &&
           second_position.y + second.top <=
               first_position.y + first.bottom;
}

bool movementPositionIsWalkable(
    const GroundMap& ground,
    const ObjectMap& objects,
    WorldPosition position,
    const ObjectBounds& bounds,
    const std::vector<MovementBlocker>* dynamic_blockers) {
    if (!positionIsWalkable(
            ground, objects, position, bounds)) {
        return false;
    }
    if (!dynamic_blockers) {
        return true;
    }
    for (const MovementBlocker& blocker : *dynamic_blockers) {
        if (boundsOverlap(
                position,
                bounds,
                blocker.position,
                blocker.bounds)) {
            return false;
        }
    }
    return true;
}

std::vector<WorldPosition> rasterizedSegment(
    WorldPosition start,
    WorldPosition end) {
    std::vector<WorldPosition> result;
    const std::int32_t delta_x = end.x - start.x;
    const std::int32_t delta_y = end.y - start.y;
    const bool horizontal =
        std::abs(delta_y) < std::abs(delta_x);
    const std::int32_t count =
        horizontal ? std::abs(delta_x) : std::abs(delta_y);
    result.reserve(static_cast<std::size_t>(count) + 1);
    if (count == 0) {
        result.push_back(start);
        return result;
    }

    if (horizontal) {
        const std::int32_t increment = delta_x < 0 ? -1 : 1;
        for (std::int32_t x = start.x;; x += increment) {
            std::int32_t y;
            if (x == start.x) {
                y = start.y;
            } else if (x == end.x) {
                y = end.y;
            } else {
                y = (x - start.x) * delta_y / delta_x + start.y;
                y = std::clamp(
                    y,
                    std::min(start.y, end.y),
                    std::max(start.y, end.y));
            }
            result.push_back({x, y});
            if (x == end.x) {
                break;
            }
        }
    } else {
        const std::int32_t increment = delta_y < 0 ? -1 : 1;
        for (std::int32_t y = start.y;; y += increment) {
            std::int32_t x;
            if (y == start.y) {
                x = start.x;
            } else if (y == end.y) {
                x = end.x;
            } else {
                x = (y - start.y) * delta_x / delta_y + start.x;
                x = std::clamp(
                    x,
                    std::min(start.x, end.x),
                    std::max(start.x, end.x));
            }
            result.push_back({x, y});
            if (y == end.y) {
                break;
            }
        }
    }
    return result;
}

struct SweepResult {
    WorldPosition position;
    bool collided = false;
};

SweepResult sweepMovement(
    const GroundMap& ground,
    const ObjectMap& objects,
    const ObjectBounds& bounds,
    WorldPosition start,
    WorldPosition end,
    std::int32_t wall_direction,
    const std::vector<MovementBlocker>* dynamic_blockers) {
    const std::vector<WorldPosition> path =
        rasterizedSegment(start, end);
    WorldPosition contact = start;
    std::size_t contact_index = 0;
    bool collided = false;
    for (std::size_t index = 1; index < path.size(); ++index) {
        if (!movementPositionIsWalkable(
                ground,
                objects,
                path[index],
                bounds,
                dynamic_blockers)) {
            collided = true;
            break;
        }
        contact = path[index];
        contact_index = index;
    }

    const bool horizontal =
        start.y == end.y && start.x != end.x;
    const bool vertical =
        start.x == end.x && start.y != end.y;
    const bool valid_wall =
        (horizontal &&
         (wall_direction == kNorth ||
          wall_direction == kSouth)) ||
        (vertical &&
         (wall_direction == kWest ||
          wall_direction == kEast));
    if (!valid_wall) {
        return {contact, collided};
    }

    const WorldPosition wall = directionVector(wall_direction);
    bool side_blocked = false;
    for (std::size_t index = 0; index < path.size(); ++index) {
        const WorldPosition side{
            path[index].x + wall.x,
            path[index].y + wall.y,
        };
        if (!movementPositionIsWalkable(
                ground,
                objects,
                side,
                bounds,
                dynamic_blockers)) {
            side_blocked = true;
            break;
        }
    }

    std::size_t nudge_index = contact_index;
    if (side_blocked) {
        std::size_t first_side_open = path.size();
        for (std::size_t index = 0;
             index <= contact_index;
             ++index) {
            const WorldPosition side{
                path[index].x + wall.x,
                path[index].y + wall.y,
            };
            if (movementPositionIsWalkable(
                    ground,
                    objects,
                    side,
                    bounds,
                    dynamic_blockers)) {
                first_side_open = index;
                break;
            }
        }
        if (first_side_open == path.size()) {
            return {contact, collided};
        }
        nudge_index = first_side_open;
    }
    const WorldPosition nudged{
        path[nudge_index].x + wall.x,
        path[nudge_index].y + wall.y,
    };
    if (movementPositionIsWalkable(
            ground,
            objects,
            nudged,
            bounds,
            dynamic_blockers)) {
        contact = nudged;
    }
    return {contact, collided || !samePosition(contact, end)};
}

bool canMoveOneUnit(
    const GroundMap& ground,
    const ObjectMap& objects,
    const ObjectBounds& bounds,
    WorldPosition position,
    std::int32_t direction,
    const std::vector<MovementBlocker>* dynamic_blockers) {
    const WorldPosition vector = directionVector(direction);
    return movementPositionIsWalkable(
        ground,
        objects,
        {position.x + vector.x, position.y + vector.y},
        bounds,
        dynamic_blockers);
}

struct ObstacleState {
    std::int32_t movement = 0;
    std::int32_t wall = 0;
};

ObstacleState initialObstacleState(
    const GroundMap& ground,
    const ObjectMap& objects,
    const ObjectBounds& bounds,
    WorldPosition start,
    WorldPosition attempted,
    WorldPosition contact,
    const std::vector<MovementBlocker>* dynamic_blockers) {
    const auto can_move =
        [&](std::int32_t direction) {
            return canMoveOneUnit(
                ground,
                objects,
                bounds,
                start,
                direction,
                dynamic_blockers);
        };
    const bool stopped = samePosition(start, contact);

    if (attempted.x > start.x && attempted.y > start.y) {
        if (stopped) {
            if (can_move(kEast)) return {kEast, kSouth};
            if (can_move(kSouth)) return {kSouth, kEast};
            if (can_move(kWest)) return {kWest, kSouth};
            if (can_move(kNorth)) return {kNorth, kEast};
            return {};
        }
        if (contact.x == start.x && contact.y != start.y) {
            return {kSouth, kEast};
        }
        if (contact.y == start.y && contact.x != start.x) {
            return {kEast, kSouth};
        }
    } else if (attempted.x > start.x && attempted.y < start.y) {
        if (stopped) {
            if (can_move(kEast)) return {kEast, kNorth};
            if (can_move(kNorth)) return {kNorth, kEast};
            if (can_move(kWest)) return {kWest, kNorth};
            if (can_move(kSouth)) return {kSouth, kEast};
            return {};
        }
        if (contact.x == start.x && contact.y != start.y) {
            return {kNorth, kEast};
        }
        if (contact.y == start.y && contact.x != start.x) {
            return {kEast, kNorth};
        }
    } else if (attempted.x < start.x && attempted.y > start.y) {
        if (stopped) {
            if (can_move(kWest)) return {kWest, kSouth};
            if (can_move(kSouth)) return {kSouth, kWest};
            if (can_move(kEast)) return {kEast, kSouth};
            if (can_move(kNorth)) return {kNorth, kWest};
            return {};
        }
        if (contact.x == start.x && contact.y != start.y) {
            return {kSouth, kWest};
        }
        if (contact.y == start.y && contact.x != start.x) {
            return {kWest, kSouth};
        }
    } else if (attempted.x < start.x && attempted.y < start.y) {
        if (stopped) {
            if (can_move(kWest)) return {kWest, kNorth};
            if (can_move(kNorth)) return {kNorth, kWest};
            if (can_move(kEast)) return {kEast, kNorth};
            if (can_move(kSouth)) return {kSouth, kWest};
            return {};
        }
        if (contact.x == start.x && contact.y != start.y) {
            return {kNorth, kWest};
        }
        if (contact.y == start.y && contact.x != start.x) {
            return {kWest, kNorth};
        }
    } else if (attempted.x == start.x &&
               attempted.y > start.y &&
               contact.y == start.y) {
        if (can_move(kEast)) return {kEast, kSouth};
        if (can_move(kWest)) return {kWest, kSouth};
    } else if (attempted.x == start.x &&
               attempted.y < start.y &&
               contact.y == start.y) {
        if (can_move(kEast)) return {kEast, kNorth};
        if (can_move(kWest)) return {kWest, kNorth};
    } else if (attempted.y == start.y &&
               attempted.x > start.x &&
               contact.x == start.x) {
        if (can_move(kSouth)) return {kSouth, kEast};
        if (can_move(kNorth)) return {kNorth, kEast};
    } else if (attempted.y == start.y &&
               attempted.x < start.x &&
               contact.x == start.x) {
        if (can_move(kSouth)) return {kSouth, kWest};
        if (can_move(kNorth)) return {kNorth, kWest};
    }
    return {};
}

bool shouldStopEdgeFollowing(
    WorldPosition position,
    WorldPosition destination,
    std::int32_t movement,
    std::int32_t wall) {
    const std::int32_t dx = destination.x - position.x;
    const std::int32_t dy = destination.y - position.y;
    if (dy < 0) {
        if (dx < 0) {
            return (movement == kSouth && wall == kEast) ||
                   (movement == kEast && wall == kSouth);
        }
        if (dx > 0) {
            return (movement == kSouth && wall == kWest) ||
                   (movement == kWest && wall == kSouth);
        }
        return movement == kSouth;
    }
    if (dy > 0) {
        if (dx < 0) {
            return (movement == kNorth && wall == kEast) ||
                   (movement == kEast && wall == kNorth);
        }
        if (dx > 0) {
            return (movement == kNorth && wall == kWest) ||
                   (movement == kWest && wall == kNorth);
        }
        return movement == kNorth;
    }
    if (dx < 0) {
        return movement == kEast;
    }
    if (dx > 0) {
        return movement == kWest;
    }
    return true;
}

ObstacleState stateAfterSideStep(
    WorldPosition position,
    WorldPosition destination,
    std::int32_t wall) {
    const std::int32_t dx = destination.x - position.x;
    const std::int32_t dy = destination.y - position.y;
    if (dy < 0 && dx < 0) {
        if (wall == kEast) return {kEast, kNorth};
        if (wall == kSouth) return {kSouth, kWest};
    } else if (dy < 0 && dx > 0) {
        if (wall == kWest) return {kWest, kNorth};
        if (wall == kSouth) return {kSouth, kEast};
    } else if (dy > 0 && dx < 0) {
        if (wall == kEast) return {kEast, kSouth};
        if (wall == kNorth) return {kNorth, kWest};
    } else if (dy > 0 && dx > 0) {
        if (wall == kWest) return {kWest, kSouth};
        if (wall == kNorth) return {kNorth, kEast};
    }
    return {};
}

}  // namespace

void MovementController::reset() {
    obstacle_direction_ = 0;
    wall_direction_ = 0;
}

MovementStepResult MovementController::advance(
    const GroundMap& ground,
    const ObjectMap& objects,
    const ObjectBounds& bounds,
    WorldPosition position,
    WorldPosition destination,
    std::int32_t speed,
    const std::vector<MovementBlocker>* dynamic_blockers) {
    if (samePosition(position, destination)) {
        reset();
        return {position, true, false};
    }
    if (speed <= 0) {
        reset();
        return {position, false, false};
    }

    if (obstacle_direction_ == 0) {
        const WorldPosition attempted =
            movementCandidate(position, destination, speed);
        const SweepResult direct =
            sweepMovement(
                ground,
                objects,
                bounds,
                position,
                attempted,
                0,
                dynamic_blockers);
        if (!direct.collided) {
            return {
                direct.position,
                samePosition(direct.position, destination),
                !samePosition(direct.position, position),
                true,
                false,
            };
        }

        const ObstacleState state =
            initialObstacleState(
                ground,
                objects,
                bounds,
                position,
                attempted,
                direct.position,
                dynamic_blockers);
        if (state.movement == 0) {
            if (!samePosition(direct.position, position)) {
                return {
                    direct.position,
                    false,
                    true,
                    true,
                    true,
                };
            }
            reset();
            return {position, false, false, false, true};
        }
        obstacle_direction_ = state.movement;
        wall_direction_ = state.wall;
        return {
            direct.position,
            false,
            !samePosition(direct.position, position),
            true,
            true,
        };
    }

    if (shouldStopEdgeFollowing(
            position,
            destination,
            obstacle_direction_,
            wall_direction_)) {
        reset();
        return {position, false, false};
    }

    const WorldPosition movement =
        directionVector(obstacle_direction_);
    const WorldPosition attempted{
        position.x + movement.x * speed,
        position.y + movement.y * speed,
    };
    const SweepResult edge =
        sweepMovement(
            ground,
            objects,
            bounds,
            position,
            attempted,
            wall_direction_,
            dynamic_blockers);
    if (samePosition(edge.position, position)) {
        const std::int32_t previous_movement =
            obstacle_direction_;
        obstacle_direction_ =
            oppositeDirection(wall_direction_);
        wall_direction_ = previous_movement;
        return {position, false, false, true, edge.collided};
    }

    const bool moved_on_zero_x =
        movement.x == 0 && edge.position.x != position.x;
    const bool moved_on_zero_y =
        movement.y == 0 && edge.position.y != position.y;
    if (moved_on_zero_x || moved_on_zero_y) {
        const ObstacleState next =
            stateAfterSideStep(
                edge.position,
                destination,
                wall_direction_);
        obstacle_direction_ = next.movement;
        wall_direction_ = next.wall;
    }
    return {
        edge.position,
        samePosition(edge.position, destination),
        true,
        true,
        edge.collided,
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
    std::int32_t speed,
    const std::vector<MovementBlocker>* dynamic_blockers) {
    if (samePosition(position, destination)) {
        return {position, true, false};
    }
    const WorldPosition candidate =
        movementCandidate(
            position, destination, std::max(speed, 0));
    const SweepResult movement =
        sweepMovement(
            ground,
            objects,
            bounds,
            position,
            candidate,
            0,
            dynamic_blockers);
    return {
        movement.position,
        samePosition(movement.position, destination),
        !samePosition(movement.position, position),
        false,
        movement.collided,
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
