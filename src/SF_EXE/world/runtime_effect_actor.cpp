#include "runtime_effect_actor.hpp"

#include "core/retail_integer.hpp"
#include "movement_controller.hpp"
#include "resources/effect_visual_resource.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace osf {
namespace {

const gapi::CafChart* selectedChart(
    const EffectVisualResource& visual,
    std::int32_t chart) {
    if (chart < 0 ||
        static_cast<std::size_t>(chart) >=
            visual.animation().charts().size()) {
        return nullptr;
    }
    return &visual.animation().charts()[
        static_cast<std::size_t>(chart)];
}

const gapi::CafDirection* selectedDirection(
    const EffectVisualResource& visual,
    std::int32_t chart,
    std::int32_t direction) {
    const gapi::CafChart* selected =
        selectedChart(visual, chart);
    if (!selected || direction < 0 || direction >= 9) {
        return nullptr;
    }
    return &selected->directions[
        static_cast<std::size_t>(direction)];
}

WorldPosition projectedPosition(
    WorldPosition position,
    double direction_radians,
    std::int32_t distance) {
    return {
        retailAdd(
            position.x,
            static_cast<std::int32_t>(
                std::cos(direction_radians) * distance)),
        retailSubtract(
            position.y,
            static_cast<std::int32_t>(
                std::sin(direction_radians) * distance)),
    };
}

bool collisionWindowActive(
    const RuntimeEffectActorSpawnRequest& request,
    std::int32_t counter) {
    return request.target_collision_start != -1 &&
           request.target_collision_start <= counter &&
           (request.target_collision_end == -1 ||
            counter <= request.target_collision_end);
}

}  // namespace

bool RuntimeEffectActor::initialize(
    const RuntimeEffectActorSpawnRequest& request,
    const EffectVisualResource& visual) {
    *this = {};
    const gapi::CafDirection* direction =
        selectedDirection(
            visual,
            request.animation_chart,
            request.animation_direction);
    if (request.resource_id < 0 ||
        !direction || direction->frame_count < 1) {
        return false;
    }

    request_ = request;
    start_position_ = request.position;
    position_ = request.position;
    previous_position_ = request.position;
    lifetime_ =
        request.lifetime_from_animation
            ? direction->frame_count
            : request.lifetime;
    visual_ = &visual;
    return true;
}

RuntimeEffectActorUpdate RuntimeEffectActor::update(
    const GroundMap& ground,
    const ObjectMap& objects) {
    RuntimeEffectActorUpdate result;
    result.intended_position = position_;
    if (expired_ || !visual_) {
        result.expired = true;
        return result;
    }

    previous_position_ = position_;
    result.target_collision_active =
        collisionWindowActive(request_, counter_);

    const std::int32_t distance =
        retailMultiply(
            request_.travel_speed,
            movement_counter_);
    result.intended_position =
        projectedPosition(
            start_position_,
            request_.direction_radians,
            distance);
    movement_counter_ =
        retailAdd(movement_counter_, 1);

    if (request_.collide_with_environment) {
        const bool start_is_clear =
            positionIsWalkable(
                ground,
                objects,
                position_,
                request_.judgement,
                true);
        const LinearMovementStep movement =
            advanceLinearMovement(
                ground,
                objects,
                request_.judgement,
                position_,
                result.intended_position,
                true);
        position_ = movement.position;
        result.environment_collision =
            !start_is_clear || movement.collided;
        if (result.environment_collision &&
            request_.expire_on_environment_collision) {
            expired_ = true;
        }
    } else {
        position_ = result.intended_position;
    }

    const gapi::CafChart* chart =
        selectedChart(
            *visual_, request_.animation_chart);
    const gapi::CafDirection* direction =
        selectedDirection(
            *visual_,
            request_.animation_chart,
            request_.animation_direction);
    animation_frame_ =
        retailMultiply(
            request_.animation_speed,
            counter_) /
        1000;
    if (chart && direction &&
        (chart->status & 1) == 0) {
        animation_frame_ = std::min(
            animation_frame_,
            static_cast<std::int32_t>(
                direction->frame_count - 1));
    }

    counter_ = retailAdd(counter_, 1);
    if (lifetime_ != -1 &&
        lifetime_ <= counter_) {
        expired_ = true;
    }
    result.expired = expired_;
    return result;
}

std::int32_t
RuntimeEffectActor::controllerEffectNumber() const {
    return request_.controller_effect_number;
}

std::int32_t RuntimeEffectActor::resourceId() const {
    return request_.resource_id;
}

WorldPosition RuntimeEffectActor::position() const {
    return position_;
}

WorldPosition RuntimeEffectActor::previousPosition() const {
    return previous_position_;
}

WorldPosition RuntimeEffectActor::renderPosition(
    double alpha) const {
    return interpolateWorldPosition(
        previous_position_, position_, alpha);
}

const ObjectBounds&
RuntimeEffectActor::judgement() const {
    return request_.judgement;
}

std::int32_t RuntimeEffectActor::animationChart() const {
    return request_.animation_chart;
}

std::int32_t
RuntimeEffectActor::animationDirection() const {
    return request_.animation_direction;
}

std::int32_t RuntimeEffectActor::animationFrame() const {
    return animation_frame_;
}

std::int32_t RuntimeEffectActor::displayHeight() const {
    return request_.display_height;
}

std::int32_t RuntimeEffectActor::counter() const {
    return counter_;
}

std::int32_t RuntimeEffectActor::movementCounter() const {
    return movement_counter_;
}

std::int32_t RuntimeEffectActor::lifetime() const {
    return lifetime_;
}

bool RuntimeEffectActor::visible() const {
    return request_.visible;
}

bool RuntimeEffectActor::expired() const {
    return expired_;
}

bool RuntimeEffectActor::partEnabled(
    std::size_t part) const {
    return visual_ &&
           part < visual_->animation().maxPartCount();
}

const gapi::NjpImage&
RuntimeEffectActor::patterns() const {
    return visual_->patterns();
}

const gapi::CafAnimation&
RuntimeEffectActor::animation() const {
    return visual_->animation();
}

}  // namespace osf
