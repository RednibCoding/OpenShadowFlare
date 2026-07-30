#include "player_actor.hpp"
#include "movement_controller.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

namespace osf {
namespace {

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

}  // namespace

void PlayerActor::reset(
    WorldPosition position,
    std::int32_t direction,
    std::int32_t walking_speed_tier) {
    position_ = position;
    previous_position_ = position;
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
    movement_controller_.reset();
}

void PlayerActor::clear() {
    reset({}, 0);
}

void PlayerActor::relocate(
    WorldPosition position,
    std::int32_t direction) {
    position_ = position;
    previous_position_ = position;
    destination_ = position;
    direction_ = direction;
    action_counter_ = 0;
    animation_chart_ = 0;
    animation_frame_ = 0;
    motion_ = PlayerMotion::idle;
    previous_action_ = PlayerMotion::idle;
    movement_controller_.reset();
}

void PlayerActor::moveTo(WorldPosition destination) {
    destination_ = destination;
    motion_ =
        movement_pace_ == MovementPace::run
            ? PlayerMotion::running
            : PlayerMotion::walking;
}

void PlayerActor::followTo(WorldPosition destination) {
    destination_ = destination;
    motion_ =
        movement_pace_ == MovementPace::run
            ? PlayerMotion::running
            : PlayerMotion::walking;
}

void PlayerActor::cancelMovement() {
    destination_ = position_;
    motion_ = PlayerMotion::idle;
    movement_controller_.reset();
}

void PlayerActor::faceToward(WorldPosition position) {
    if (position.x == position_.x &&
        position.y == position_.y) {
        return;
    }
    direction_ = retailDirectionForVector(
        position.x - position_.x,
        position.y - position_.y);
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
    const ObjectMap& objects,
    const std::vector<MovementBlocker>* dynamic_blockers) {
    previous_position_ = position_;
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
    const MovementStepResult movement =
        movement_controller_.advance(
            ground,
            objects,
            judgement_,
            position_,
            destination_,
            moving_action == PlayerMotion::running
                ? running_speed_
                : walking_speed_,
            dynamic_blockers);
    if (movement.moved) {
        direction_ = retailDirectionForVector(
            movement.position.x - position_.x,
            movement.position.y - position_.y);
    }
    position_ = movement.position;
    if (movement.moved || movement.controller_active) {
        return;
    }
    motion_ = PlayerMotion::idle;
}

WorldPosition PlayerActor::position() const {
    return position_;
}

WorldPosition PlayerActor::renderPosition(double alpha) const {
    return interpolateWorldPosition(
        previous_position_, position_, alpha);
}

WorldPosition PlayerActor::destination() const {
    return destination_;
}

const ObjectBounds& PlayerActor::judgement() const {
    return judgement_;
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
