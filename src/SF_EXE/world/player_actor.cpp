#include "player_actor.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace osf {
namespace {

constexpr double kRadiansToTenthsOfDegrees =
    572.95779513082320877;

constexpr std::array<double, 10> kSpeedFactors{{
    2.0 / 3.0,
    3.0 / 4.0,
    4.0 / 5.0,
    5.0 / 6.0,
    6.0 / 7.0,
    1.0,
    7.0 / 6.0,
    6.0 / 5.0,
    5.0 / 4.0,
    4.0 / 3.0,
}};

double speedFactor(std::int32_t tier) {
    return kSpeedFactors[
        static_cast<std::size_t>(
            std::clamp(tier, 0, 9))];
}

std::int32_t walkingSpeedForTier(std::int32_t tier) {
    // FUN_00450080 multiplies the tier factor by the retail base speed 20.0
    // before truncating it into the movement controller's integer step.
    return static_cast<std::int32_t>(
        speedFactor(tier) * 20.0);
}

std::int32_t animationFrameForSpeed(
    std::int32_t counter,
    std::int32_t tier) {
    return static_cast<std::int32_t>(
        static_cast<double>(counter) *
        speedFactor(tier));
}

WorldPosition movementStep(
    WorldPosition current,
    WorldPosition destination,
    std::int32_t speed) {
    const std::int64_t delta_x =
        static_cast<std::int64_t>(destination.x) - current.x;
    const std::int64_t delta_y =
        static_cast<std::int64_t>(destination.y) - current.y;
    const double distance = std::hypot(
        static_cast<double>(delta_x),
        static_cast<double>(delta_y));
    if (distance < 1.0) {
        return destination;
    }

    WorldPosition result{
        current.x + static_cast<std::int32_t>(
            static_cast<double>(delta_x) /
            distance * speed),
        current.y + static_cast<std::int32_t>(
            static_cast<double>(delta_y) /
            distance * speed),
    };
    if ((delta_x > 0 && result.x > destination.x) ||
        (delta_x < 0 && result.x < destination.x)) {
        result.x = destination.x;
    }
    if ((delta_y > 0 && result.y > destination.y) ||
        (delta_y < 0 && result.y < destination.y)) {
        result.y = destination.y;
    }
    return result;
}

WorldPosition furthestWalkablePosition(
    const GroundMap& ground,
    const ObjectMap& objects,
    const ObjectBounds& bounds,
    WorldPosition start,
    WorldPosition end,
    bool* reached) {
    const std::int32_t delta_x = end.x - start.x;
    const std::int32_t delta_y = end.y - start.y;
    const std::int32_t steps = std::max(
        std::abs(delta_x), std::abs(delta_y));
    WorldPosition result = start;
    *reached = true;
    for (std::int32_t step = 1; step <= steps; ++step) {
        const WorldPosition position{
            start.x + delta_x * step / steps,
            start.y + delta_y * step / steps,
        };
        if (!positionIsWalkable(
                ground, objects, position, bounds)) {
            *reached = false;
            break;
        }
        result = position;
    }
    return result;
}

std::int32_t squaredDistance(
    WorldPosition first,
    WorldPosition second) {
    const std::int32_t x = first.x - second.x;
    const std::int32_t y = first.y - second.y;
    return x * x + y * y;
}

}  // namespace

std::int32_t retailDirectionForVector(
    std::int32_t x,
    std::int32_t y) {
    double angle = std::atan2(
        -static_cast<double>(y),
        static_cast<double>(x));
    constexpr double full_turn =
        6.28318530717958647692;
    while (angle < 0.0) {
        angle += full_turn;
    }
    while (angle > full_turn) {
        angle -= full_turn;
    }
    const std::int32_t tenths =
        static_cast<std::int32_t>(
            angle * kRadiansToTenthsOfDegrees) %
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

void PlayerActor::reset(
    WorldPosition position,
    std::int32_t direction,
    std::int32_t walking_speed_tier) {
    position_ = position;
    destination_ = position;
    direction_ = direction;
    walking_speed_tier_ =
        std::clamp(walking_speed_tier, 0, 9);
    walking_speed_ =
        walkingSpeedForTier(walking_speed_tier_);
    running_speed_ = walking_speed_ * 2;
    action_counter_ = 0;
    animation_chart_ = 0;
    animation_frame_ = 0;
    movement_pace_ = MovementPace::walk;
    motion_ = PlayerMotion::idle;
    previous_action_ = PlayerMotion::idle;
}

void PlayerActor::clear() {
    reset({}, 0);
}

void PlayerActor::moveTo(WorldPosition destination) {
    destination_ = destination;
    motion_ =
        movement_pace_ == MovementPace::run
            ? PlayerMotion::running
            : PlayerMotion::walking;
}

void PlayerActor::cancelMovement() {
    destination_ = position_;
    motion_ = PlayerMotion::idle;
}

void PlayerActor::toggleMovementPace() {
    movement_pace_ =
        movement_pace_ == MovementPace::walk
            ? MovementPace::run
            : MovementPace::walk;
    if (motion_ != PlayerMotion::idle) {
        motion_ =
            movement_pace_ == MovementPace::run
                ? PlayerMotion::running
                : PlayerMotion::walking;
    }
}

void PlayerActor::update(
    const GroundMap& ground,
    const ObjectMap& objects) {
    if (motion_ == PlayerMotion::idle) {
        if (previous_action_ != PlayerMotion::idle) {
            action_counter_ = 0;
        }
        animation_chart_ = 0;
        animation_frame_ = action_counter_;
        ++action_counter_;
        previous_action_ = PlayerMotion::idle;
        return;
    }

    const PlayerMotion moving_action = motion_;
    if (previous_action_ != moving_action) {
        action_counter_ = 0;
    } else {
        ++action_counter_;
    }
    animation_chart_ =
        moving_action == PlayerMotion::running ? 2 : 1;
    animation_frame_ = animationFrameForSpeed(
        action_counter_, walking_speed_tier_);
    previous_action_ = moving_action;

    if (position_.x == destination_.x &&
        position_.y == destination_.y) {
        motion_ = PlayerMotion::idle;
        return;
    }

    direction_ = retailDirectionForVector(
        destination_.x - position_.x,
        destination_.y - position_.y);
    const WorldPosition start = position_;
    const WorldPosition candidate =
        movementStep(
            position_,
            destination_,
            moving_action == PlayerMotion::running
                ? running_speed_
                : walking_speed_);
    bool reached = false;
    const WorldPosition direct =
        furthestWalkablePosition(
            ground,
            objects,
            judgement_,
            position_,
            candidate,
            &reached);
    if (reached) {
        position_ = direct;
        return;
    }

    bool ignored = false;
    const WorldPosition x_slide =
        furthestWalkablePosition(
            ground,
            objects,
            judgement_,
            direct,
            {candidate.x, direct.y},
            &ignored);
    const WorldPosition y_slide =
        furthestWalkablePosition(
            ground,
            objects,
            judgement_,
            direct,
            {direct.x, candidate.y},
            &ignored);
    position_ =
        squaredDistance(start, x_slide) >=
                squaredDistance(start, y_slide)
            ? x_slide
            : y_slide;
    if (position_.x != start.x ||
        position_.y != start.y) {
        return;
    }
    motion_ = PlayerMotion::idle;
}

WorldPosition PlayerActor::position() const {
    return position_;
}

WorldPosition PlayerActor::destination() const {
    return destination_;
}

std::int32_t PlayerActor::direction() const {
    return direction_;
}

std::int32_t PlayerActor::walkingSpeedTier() const {
    return walking_speed_tier_;
}

std::int32_t PlayerActor::walkingSpeed() const {
    return walking_speed_;
}

std::int32_t PlayerActor::runningSpeed() const {
    return running_speed_;
}

MovementPace PlayerActor::movementPace() const {
    return movement_pace_;
}

PlayerMotion PlayerActor::motion() const {
    return motion_;
}

std::int32_t PlayerActor::animationChart() const {
    return animation_chart_;
}

std::int32_t PlayerActor::animationFrame() const {
    return animation_frame_;
}

}  // namespace osf
