#include "enemy_effect_controller.hpp"

#include "actor_direction.hpp"
#include "core/retail_integer.hpp"
#include "core/retail_random.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"

#include <cmath>
#include <cstdint>

namespace osf {
namespace {

constexpr std::int32_t kTypeOneEffect = 10001;
constexpr std::int32_t kTypeTwoEffect = 10002;
constexpr std::int32_t kTypeThreeEffect = 10003;
constexpr std::int32_t kTypeFourEffect = 10004;
constexpr std::int32_t kTypeOneSourceResource = 10000012;
constexpr std::int32_t kTypeTwoSourceResource = 11000027;
constexpr std::int32_t kTypeOneChildResource = 10000010;
constexpr std::int32_t kTypeTwoChildResource = 10000040;
constexpr std::int32_t kTypeThreeFirstResource = 10000030;
constexpr std::int32_t kTypeThreeSecondResource = 10000031;
constexpr std::int32_t kTypeThreeThirdResource = 10000032;
constexpr std::int32_t kTypeFourWarningResource = 10000002;
constexpr std::int32_t kTypeFourBurstResource = 10000000;
constexpr std::int32_t kTypeOneAudioSample = 19;
constexpr std::int32_t kTypeTwoAudioSample = 94;
constexpr std::int32_t kTypeThreeAudioSample = 21;
constexpr std::int32_t kTypeFourFirstAudioSample = 29;
constexpr std::int32_t kTypeFourSecondAudioSample = 23;
constexpr std::int32_t kChildDistance = 180;
constexpr std::int32_t kChildRadius = 50;
constexpr std::int32_t kTypeThreeFirstRadius = 250;
constexpr std::int32_t kTypeThreeRadiusStep = 200;
constexpr std::int32_t kTypeThreeWavePeriod = 4;
constexpr std::int32_t kTypeThreeRadius = 100;
constexpr std::int32_t kTypeThreeWaveTable = 205;
constexpr std::int32_t kTypeFourWarningUpdate = 3;
constexpr std::int32_t kTypeFourDisplayHeight = 200;
constexpr std::int32_t kTypeFourDamageExpansion = 150;
constexpr std::int32_t kTypeFourShakeRange = 3001;
constexpr std::int32_t kTypeFourShakeDuration = 8;
constexpr std::int32_t kTypeFourShakeMagnitude = 6;

bool supportedEffect(std::int32_t effect_number) {
    return effect_number == kTypeOneEffect ||
           effect_number == kTypeTwoEffect ||
           effect_number == kTypeThreeEffect ||
           effect_number == kTypeFourEffect;
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
    double direction_radians,
    std::int32_t distance) {
    return {
        retailAdd(
            position.x,
            static_cast<std::int32_t>(
                std::cos(direction_radians) *
                distance)),
        retailSubtract(
            position.y,
            static_cast<std::int32_t>(
                std::sin(direction_radians) *
                distance)),
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
    actor.expire_on_target = true;
    actor.remember_targets =
        request.constructor_value_22 == 1;
    actor.target_audio = {0, 20};
    actor.animation_direction =
        retailDirectionForAngle(
            request.direction_radians);
    actor.has_packet = request.has_packet;
    actor.packet = request.packet;
    return actor;
}

RuntimeEffectActorSpawnRequest typeThreeActor(
    const CombatEffectSpawnRequest& request,
    WorldPosition position,
    std::int32_t resource_id,
    std::int32_t animation_chart,
    bool damaging_layer) {
    RuntimeEffectActorSpawnRequest actor;
    actor.controller_effect_number =
        request.effect_number;
    actor.resource_id = resource_id;
    actor.owner_kind = request.owner_kind;
    actor.source_character_number =
        request.source_character_number;
    actor.target_mask = request.target_kind;
    actor.target_identifier = 0;
    actor.position = position;
    actor.judgement = {
        -kTypeThreeRadius,
        -kTypeThreeRadius,
        kTypeThreeRadius,
        kTypeThreeRadius,
    };
    actor.lifetime_from_animation = true;
    actor.target_collision_start =
        damaging_layer ? 0 : -1;
    actor.target_collision_end = 0;
    actor.process_every_target = damaging_layer;
    actor.animation_chart = animation_chart;
    actor.animation_direction = 8;
    actor.has_packet = request.has_packet;
    actor.packet = request.packet;
    return actor;
}

RuntimeEffectActorSpawnRequest typeFourVisual(
    const CombatEffectSpawnRequest& request,
    WorldPosition position,
    ObjectBounds judgement,
    std::int32_t resource_id,
    std::int32_t chart,
    std::int32_t lifetime_chart,
    std::int32_t display_height,
    std::int32_t additional_status) {
    RuntimeEffectActorSpawnRequest actor;
    actor.controller_effect_number =
        request.effect_number;
    actor.resource_id = resource_id;
    actor.owner_kind = request.owner_kind;
    actor.source_character_number =
        request.source_character_number;
    actor.position = position;
    actor.judgement = judgement;
    actor.display_height = display_height;
    actor.lifetime_from_animation = true;
    actor.lifetime_animation_chart =
        lifetime_chart;
    actor.animation_chart = chart;
    actor.animation_direction = 8;
    actor.additional_display_status =
        additional_status;
    return actor;
}

RuntimeEffectActorSpawnRequest typeFourDamageActor(
    const CombatEffectSpawnRequest& request,
    WorldPosition position) {
    const ObjectBounds source =
        request.has_source_judgement
            ? request.source_judgement
            : ObjectBounds{};
    RuntimeEffectActorSpawnRequest actor;
    actor.controller_effect_number =
        request.effect_number;
    actor.resource_id = -1;
    actor.owner_kind = request.owner_kind;
    actor.source_character_number =
        request.source_character_number;
    actor.target_mask = request.target_kind;
    actor.target_identifier =
        request.target_identifier;
    actor.position = position;
    actor.judgement = {
        retailSubtract(
            source.left,
            kTypeFourDamageExpansion),
        retailSubtract(
            source.top,
            kTypeFourDamageExpansion),
        retailAdd(
            source.right,
            kTypeFourDamageExpansion),
        retailAdd(
            source.bottom,
            kTypeFourDamageExpansion),
    };
    actor.display_height = kTypeFourDisplayHeight;
    actor.lifetime = 1;
    actor.target_collision_start = 0;
    actor.target_collision_end = 0;
    actor.process_every_target = true;
    actor.target_audio = {0, 20};
    actor.animation_chart = 0;
    actor.animation_direction = 8;
    actor.visible = false;
    actor.has_packet = request.has_packet;
    actor.packet = request.packet;
    return actor;
}

std::int32_t positionDistance(
    WorldPosition first,
    WorldPosition second) {
    return static_cast<std::int32_t>(
        std::trunc(
            std::hypot(
                static_cast<double>(first.x) - second.x,
                static_cast<double>(first.y) - second.y)));
}

}  // namespace

bool EnemyEffectController::initialize(
    const CombatEffectSpawnRequest& request,
    const TableDatabase* tables) {
    *this = {};
    if (!request.valid ||
        !supportedEffect(request.effect_number)) {
        return false;
    }
    if (request.effect_number == kTypeThreeEffect) {
        const TableData* wave_table =
            tables
                ? tables->find(kTypeThreeWaveTable)
                : nullptr;
        const std::int32_t column =
            retailSubtract(
                request.constructor_value_17, 1);
        if (!wave_table ||
            !wave_table->contains(0, column)) {
            return false;
        }
        type_three_wave_count_ =
            wave_table->value(0, column);
    }
    request_ = request;
    active_ = true;
    return true;
}

EnemyEffectControllerUpdate
EnemyEffectController::update(
    const EnemyEffectControllerContext& context) {
    EnemyEffectControllerUpdate result;
    if (!active_) {
        result.expired = true;
        return result;
    }

    if (request_.effect_number == kTypeThreeEffect) {
        if (request_.constructor_value_12 <= counter_ &&
            retailSubtract(
                counter_,
                request_.constructor_value_12) %
                    kTypeThreeWavePeriod ==
                0) {
            const std::int32_t distance =
                retailAdd(
                    retailMultiply(
                        type_three_wave_index_,
                        kTypeThreeRadiusStep),
                    kTypeThreeFirstRadius);
            const WorldPosition position =
                projectedPosition(
                    request_.has_explicit_origin
                        ? request_.origin
                        : WorldPosition{},
                    request_.direction_radians,
                    distance);
            const ObjectBounds judgement{
                -kTypeThreeRadius,
                -kTypeThreeRadius,
                kTypeThreeRadius,
                kTypeThreeRadius,
            };
            if (context.placement_is_clear &&
                !context.placement_is_clear(
                    position, judgement)) {
                type_three_placement_blocked_ = true;
            }
            if (!type_three_placement_blocked_) {
                const std::int32_t chart =
                    context.random
                        ? context.random->next() % 4
                        : 0;
                result.actor_spawns[
                    result.actor_spawn_count++] =
                    typeThreeActor(
                        request_,
                        position,
                        kTypeThreeFirstResource,
                        chart,
                        true);
                result.actor_spawns[
                    result.actor_spawn_count++] =
                    typeThreeActor(
                        request_,
                        position,
                        kTypeThreeSecondResource,
                        0,
                        false);
                result.actor_spawns[
                    result.actor_spawn_count++] =
                    typeThreeActor(
                        request_,
                        position,
                        kTypeThreeThirdResource,
                        0,
                        false);
                result.audio[result.audio_count++] = {
                    kTypeThreeAudioSample,
                    position,
                };
            }
            type_three_wave_index_ =
                retailAdd(type_three_wave_index_, 1);
        }

        counter_ = retailAdd(counter_, 1);
        if (counter_ ==
            retailAdd(
                request_.constructor_value_12,
                retailMultiply(
                    type_three_wave_count_,
                    kTypeThreeWavePeriod))) {
            active_ = false;
            result.expired = true;
        }
        return result;
    }

    if (request_.effect_number == kTypeFourEffect) {
        if (counter_ == kTypeFourWarningUpdate) {
            const ObjectBounds judgement =
                request_.has_source_judgement
                    ? request_.source_judgement
                    : ObjectBounds{};
            result.actor_spawns[
                result.actor_spawn_count++] =
                typeFourVisual(
                    request_,
                    resolvedPosition(
                        request_, context.source),
                    judgement,
                    kTypeFourWarningResource,
                    0,
                    0,
                    0,
                    0x80);
        }

        if (counter_ == request_.constructor_value_12) {
            const WorldPosition position =
                resolvedPosition(
                    request_, context.source);
            const ObjectBounds source =
                request_.has_source_judgement
                    ? request_.source_judgement
                    : ObjectBounds{};
            result.actor_spawns[
                result.actor_spawn_count++] =
                typeFourVisual(
                    request_,
                    position,
                    {
                        retailSubtract(source.left, 1),
                        retailSubtract(source.top, 1),
                        retailSubtract(source.left, 1),
                        retailSubtract(source.top, 1),
                    },
                    kTypeFourBurstResource,
                    1,
                    1,
                    kTypeFourDisplayHeight,
                    0);
            result.actor_spawns[
                result.actor_spawn_count++] =
                typeFourVisual(
                    request_,
                    position,
                    {
                        retailAdd(source.right, 1),
                        retailAdd(source.bottom, 1),
                        retailAdd(source.right, 1),
                        retailAdd(source.bottom, 1),
                    },
                    kTypeFourBurstResource,
                    0,
                    1,
                    kTypeFourDisplayHeight,
                    0);
            result.audio[result.audio_count++] = {
                kTypeFourFirstAudioSample,
                position,
            };
            result.audio[result.audio_count++] = {
                kTypeFourSecondAudioSample,
                position,
            };
            if (context.observer.found &&
                positionDistance(
                    context.observer.position,
                    position) <
                    kTypeFourShakeRange) {
                result.camera_shake = true;
                result.camera_shake_duration =
                    kTypeFourShakeDuration;
                result.camera_shake_magnitude =
                    kTypeFourShakeMagnitude;
            }
            result.actor_spawns[
                result.actor_spawn_count++] =
                typeFourDamageActor(
                    request_, position);
            active_ = false;
            result.expired = true;
            return result;
        }

        counter_ = retailAdd(counter_, 1);
        return result;
    }

    if (counter_ == 0) {
        result.actor_spawns[
            result.actor_spawn_count++] =
            sourceActor(
                request_,
                resolvedPosition(
                    request_, context.source));
    }

    if (counter_ == request_.constructor_value_12) {
        const WorldPosition position =
            projectedPosition(
                resolvedPosition(
                    request_, context.source),
                request_.direction_radians,
                kChildDistance);
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
