#include "enemy_effect_controller.hpp"

#include "actor_direction.hpp"
#include "core/retail_integer.hpp"

#include <cmath>
#include <cstdint>

namespace osf {
namespace {

constexpr std::int32_t kTypeOneEffect = 10001;
constexpr std::int32_t kTypeTwoEffect = 10002;
constexpr std::int32_t kTypeOneSourceResource = 10000012;
constexpr std::int32_t kTypeTwoSourceResource = 11000027;
constexpr std::int32_t kTypeOneChildResource = 10000010;
constexpr std::int32_t kTypeTwoChildResource = 10000040;
constexpr std::int32_t kTypeOneAudioSample = 19;
constexpr std::int32_t kTypeTwoAudioSample = 94;
constexpr std::int32_t kChildDistance = 180;
constexpr std::int32_t kChildRadius = 50;

bool supportedEffect(std::int32_t effect_number) {
    return effect_number == kTypeOneEffect ||
           effect_number == kTypeTwoEffect;
}

WorldPosition resolvedPosition(
    const CombatEffectSpawnRequest& request,
    const EnemyEffectControllerSource& source) {
    if (request.owner_kind == 0) {
        return request.has_explicit_origin
            ? request.origin
            : WorldPosition{};
    }
    return source.found
        ? source.position
        : WorldPosition{};
}

WorldPosition projectedPosition(
    WorldPosition position,
    double direction_radians) {
    return {
        retailAdd(
            position.x,
            static_cast<std::int32_t>(
                std::cos(direction_radians) *
                kChildDistance)),
        retailSubtract(
            position.y,
            static_cast<std::int32_t>(
                std::sin(direction_radians) *
                kChildDistance)),
    };
}

RuntimeEffectActorSpawnRequest sourceActor(
    const CombatEffectSpawnRequest& request,
    WorldPosition position) {
    RuntimeEffectActorSpawnRequest actor;
    actor.controller_effect_number =
        request.effect_number;
    actor.resource_id =
        request.effect_number == kTypeOneEffect
            ? kTypeOneSourceResource
            : kTypeTwoSourceResource;
    actor.owner_kind = request.owner_kind;
    actor.source_character_number =
        request.source_character_number;
    actor.position = position;
    const ObjectBounds judgement =
        request.has_source_judgement
            ? request.source_judgement
            : ObjectBounds{};
    const std::int32_t right =
        retailAdd(judgement.right, 1);
    const std::int32_t bottom =
        retailAdd(judgement.bottom, 1);
    actor.judgement = {
        right,
        bottom,
        right,
        bottom,
    };
    actor.lifetime_from_animation = true;
    return actor;
}

RuntimeEffectActorSpawnRequest childActor(
    const CombatEffectSpawnRequest& request,
    WorldPosition position) {
    RuntimeEffectActorSpawnRequest actor;
    actor.controller_effect_number =
        request.effect_number;
    actor.resource_id =
        request.effect_number == kTypeOneEffect
            ? kTypeOneChildResource
            : kTypeTwoChildResource;
    actor.owner_kind = request.owner_kind;
    actor.source_character_number =
        request.source_character_number;
    actor.target_mask = request.target_kind;
    actor.target_identifier =
        request.target_identifier;
    actor.travel_speed =
        request.constructor_value_6;
    actor.display_height =
        request.constructor_value_7;
    actor.direction_radians =
        request.direction_radians;
    actor.position = position;
    actor.judgement = {
        -kChildRadius,
        -kChildRadius,
        kChildRadius,
        kChildRadius,
    };
    actor.expire_on_environment_collision = true;
    actor.target_collision_start = 0;
    actor.animation_direction =
        retailDirectionForAngle(
            request.direction_radians);
    actor.has_packet = request.has_packet;
    actor.packet = request.packet;
    return actor;
}

}  // namespace

bool EnemyEffectController::initialize(
    const CombatEffectSpawnRequest& request) {
    *this = {};
    if (!request.valid ||
        !supportedEffect(request.effect_number)) {
        return false;
    }
    request_ = request;
    active_ = true;
    return true;
}

EnemyEffectControllerUpdate
EnemyEffectController::update(
    const EnemyEffectControllerSource& source) {
    EnemyEffectControllerUpdate result;
    if (!active_) {
        result.expired = true;
        return result;
    }

    if (counter_ == 0) {
        result.actor_spawns[
            result.actor_spawn_count++] =
            sourceActor(
                request_,
                resolvedPosition(request_, source));
    }

    if (counter_ == request_.constructor_value_12) {
        const WorldPosition position =
            projectedPosition(
                resolvedPosition(request_, source),
                request_.direction_radians);
        result.actor_spawns[
            result.actor_spawn_count++] =
            childActor(request_, position);
        result.audio[result.audio_count++] = {
            request_.effect_number == kTypeOneEffect
                ? kTypeOneAudioSample
                : kTypeTwoAudioSample,
            position,
        };
        active_ = false;
        result.expired = true;
        return result;
    }

    counter_ = retailAdd(counter_, 1);
    return result;
}

bool EnemyEffectController::active() const {
    return active_;
}

std::int32_t EnemyEffectController::counter() const {
    return counter_;
}

std::int32_t EnemyEffectController::effectNumber() const {
    return active_ ? request_.effect_number : -1;
}

}  // namespace osf
