#include "runtime_effect_actor.hpp"

#include "actor_direction.hpp"
#include "core/retail_random.hpp"
#include "core/retail_integer.hpp"
#include "movement_controller.hpp"
#include "resources/effect_visual_resource.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>

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

double normalizedRetailAngle(double angle) {
    while (angle < 0.0) {
        angle += kRetailFullCircleRadians;
    }
    while (angle > kRetailFullCircleRadians) {
        angle -= kRetailFullCircleRadians;
    }
    return angle;
}

bool livingHomingTarget(
    const RuntimeEffectTargetSnapshot& target) {
    if (!target.present || !target.same_scenario) {
        return false;
    }
    if (target.kind == RuntimeEffectTargetKind::player ||
        target.kind == RuntimeEffectTargetKind::companion ||
        target.kind == RuntimeEffectTargetKind::enemy) {
        return target.current_life > 0 &&
               (target.kind !=
                    RuntimeEffectTargetKind::enemy ||
                target.active);
    }
    return true;
}

const RuntimeEffectTargetSnapshot* homingTarget(
    const RuntimeEffectActorSpawnRequest& request,
    const std::vector<RuntimeEffectTargetSnapshot>& targets) {
    const auto found = std::find_if(
        targets.begin(),
        targets.end(),
        [&request](
            const RuntimeEffectTargetSnapshot& target) {
            if (!livingHomingTarget(target)) {
                return false;
            }
            if (request.target_mask == 1) {
                return target.kind ==
                           RuntimeEffectTargetKind::player &&
                       target.character_number ==
                           request.target_identifier;
            }
            return target.kind !=
                       RuntimeEffectTargetKind::player &&
                   target.identifier ==
                       request.target_identifier;
        });
    return found != targets.end() ? &*found : nullptr;
}

bool targetNotPassed(
    WorldPosition start,
    WorldPosition current,
    WorldPosition target) {
    return (start.x > target.x) !=
               (target.x > current.x) ||
           (start.y > target.y) !=
               (target.y > current.y);
}

}  // namespace

bool RuntimeEffectActor::initialize(
    const RuntimeEffectActorSpawnRequest& request,
    const EffectVisualResource& visual) {
    return initialize(request, &visual);
}

bool RuntimeEffectActor::initialize(
    const RuntimeEffectActorSpawnRequest& request,
    const EffectVisualResource* visual) {
    *this = {};
    const bool invisible_without_resource =
        !request.visible && request.resource_id < 0;
    const std::int32_t lifetime_chart =
        request.lifetime_animation_chart >= 0
            ? request.lifetime_animation_chart
            : request.animation_chart;
    const gapi::CafDirection* lifetime_direction =
        visual
            ? selectedDirection(
                  *visual,
                  lifetime_chart,
                  request.animation_direction)
            : nullptr;
    const gapi::CafDirection* display_direction =
        visual
            ? selectedDirection(
                  *visual,
                  request.animation_chart,
                  request.animation_direction)
            : nullptr;
    if ((!invisible_without_resource &&
         (request.resource_id < 0 ||
          !display_direction ||
          display_direction->frame_count < 1)) ||
        (request.lifetime_from_animation &&
         (!lifetime_direction ||
          lifetime_direction->frame_count < 1))) {
        return false;
    }

    request_ = request;
    start_position_ = request.position;
    position_ = request.position;
    previous_position_ = request.position;
    lifetime_ =
        request.lifetime_from_animation
            ? lifetime_direction->frame_count
            : request.lifetime;
    direction_radians_ = request.direction_radians;
    animation_direction_ = request.animation_direction;
    homing_active_ = request.home_toward_target;
    visual_ = visual;
    return true;
}

RuntimeEffectActorUpdate RuntimeEffectActor::update(
    const GroundMap& ground,
    const ObjectMap& objects) {
    RetailRandom unused_random;
    return update(
        ground,
        objects,
        {},
        unused_random);
}

RuntimeEffectActorUpdate RuntimeEffectActor::update(
    const GroundMap& ground,
    const ObjectMap& objects,
    const std::vector<RuntimeEffectTargetSnapshot>& targets,
    RetailRandom& random) {
    RuntimeEffectActorUpdate result;
    result.intended_position = position_;
    if (expired_ || (request_.visible && !visual_)) {
        result.expired = true;
        return result;
    }

    previous_position_ = position_;
    has_updated_ = true;
    if (request_.home_toward_target) {
        if (homing_active_) {
            const RuntimeEffectTargetSnapshot* target =
                homingTarget(request_, targets);
            if (!target) {
                homing_active_ = false;
            } else if (targetNotPassed(
                           start_position_,
                           position_,
                           target->position)) {
                const double desired =
                    std::atan2(
                        static_cast<double>(
                            position_.y -
                            target->position.y),
                        static_cast<double>(
                            target->position.x -
                            position_.x));
                const double difference =
                    normalizedRetailAngle(
                        desired - direction_radians_);
                const double turn =
                    static_cast<double>(
                        request_.homing_turn_speed) *
                    kRetailRadiansPerDegree;
                if (difference <= turn ||
                    kRetailFullCircleRadians - turn <=
                        difference) {
                    direction_radians_ = desired;
                } else if (
                    difference <=
                        kRetailHalfCircleRadians) {
                    direction_radians_ += turn;
                } else {
                    direction_radians_ -= turn;
                }
            }
        }
        result.intended_position =
            projectedPosition(
                position_,
                direction_radians_,
                request_.travel_speed);
    } else {
        const std::int32_t distance =
            retailMultiply(
                request_.travel_speed,
                movement_counter_);
        result.intended_position =
            projectedPosition(
                start_position_,
                direction_radians_,
                distance);
        movement_counter_ =
            retailAdd(movement_counter_, 1);
    }

    result.target_collision_active =
        collisionWindowActive(request_, counter_);
    if (result.target_collision_active) {
        RuntimeEffectTargetQuery query;
        query.actor_identifier =
            request_.actor_identifier;
        query.actor_position = position_;
        query.actor_judgement = request_.judgement;
        query.target_mask = request_.target_mask;
        query.target_identifier =
            request_.target_identifier;
        query.exact_target_only =
            request_.exact_target_only;
        query.process_every_target =
            request_.process_every_target;
        query.expire_on_target =
            request_.expire_on_target;
        query.expire_on_object_contact =
            request_.expire_on_environment_collision;
        query.remember_targets =
            request_.remember_targets;
        query.has_packet = request_.has_packet;
        query.magical_evasion =
            request_.has_packet &&
            request_.packet[1] == 3;
        query.hit_rating =
            request_.has_packet
                ? request_.packet[36]
                : 0;
        query.target_audio = request_.target_audio;
        query.object_audio =
            request_.environment_audio;
        RuntimeEffectTargetResolution target_result =
            resolveRuntimeEffectTargets(
                query,
                targets,
                target_memory_,
                random);
        result.target_contacts =
            std::move(target_result.contacts);
        result.audio =
            std::move(target_result.audio);
        if (target_result.expired) {
            expired_ = true;
        }
    }

    if (request_.collide_with_environment) {
        const WorldPosition collision_audio_position =
            position_;
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
            const bool was_expired = expired_;
            expired_ = true;
            if ((!was_expired ||
                 !start_is_clear) &&
                request_.environment_audio.bank != -1) {
                result.audio.push_back({
                    request_.environment_audio,
                    collision_audio_position,
                    false,
                });
            }
        }
    } else {
        position_ = result.intended_position;
    }

    if (visual_) {
        if (request_.home_toward_target) {
            animation_direction_ =
                retailDirectionForAngle(
                    direction_radians_);
        }
        const gapi::CafChart* chart =
            selectedChart(
                *visual_, request_.animation_chart);
        const gapi::CafDirection* direction =
            selectedDirection(
                *visual_,
                request_.animation_chart,
                animation_direction_);
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

std::int32_t RuntimeEffectActor::ownerKind() const {
    return request_.owner_kind;
}

std::int32_t
RuntimeEffectActor::sourceCharacterNumber() const {
    return request_.source_character_number;
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
    return animation_direction_;
}

std::int32_t RuntimeEffectActor::animationFrame() const {
    return animation_frame_;
}

std::int32_t RuntimeEffectActor::displayHeight() const {
    return request_.display_height;
}

std::int32_t
RuntimeEffectActor::additionalDisplayStatus() const {
    return request_.additional_display_status;
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

bool RuntimeEffectActor::hasUpdated() const {
    return has_updated_;
}

bool RuntimeEffectActor::targetCollisionActive() const {
    return !expired_ &&
           (visual_ || !request_.visible) &&
           collisionWindowActive(request_, counter_);
}

bool RuntimeEffectActor::needsTargetSnapshots() const {
    return targetCollisionActive() ||
           (!expired_ &&
            request_.home_toward_target &&
            homing_active_);
}

bool RuntimeEffectActor::hasPacket() const {
    return request_.has_packet;
}

const CombatPacket& RuntimeEffectActor::packet() const {
    return request_.packet;
}

std::size_t
RuntimeEffectActor::rememberedTargetCount() const {
    return target_memory_.count();
}

bool RuntimeEffectActor::partEnabled(
    std::size_t part) const {
    return visual_ &&
           part < visual_->animation().maxPartCount();
}

const gapi::NjpImage&
RuntimeEffectActor::patterns() const {
    static const gapi::NjpImage empty;
    return visual_ ? visual_->patterns() : empty;
}

const gapi::CafAnimation&
RuntimeEffectActor::animation() const {
    static const gapi::CafAnimation empty;
    return visual_ ? visual_->animation() : empty;
}

}  // namespace osf
