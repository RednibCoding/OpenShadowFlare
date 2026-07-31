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
constexpr std::int32_t kTypeFiveEffect = 10005;
constexpr std::int32_t kTypeTenEffect = 10010;
constexpr std::int32_t kTypeElevenEffect = 10011;
constexpr std::int32_t kTypeTwelveEffect = 10012;
constexpr std::int32_t kTypeThirteenEffect = 10013;
constexpr std::int32_t kTypeFourteenEffect = 10014;
constexpr std::int32_t kTypeSixteenEffect = 10016;
constexpr std::int32_t kTypeTwentyOneEffect = 10021;
constexpr std::int32_t kTypeOneSourceResource = 10000012;
constexpr std::int32_t kTypeTwoSourceResource = 11000027;
constexpr std::int32_t kTypeOneChildResource = 10000010;
constexpr std::int32_t kTypeTwoChildResource = 10000040;
constexpr std::int32_t kTypeThreeFirstResource = 10000030;
constexpr std::int32_t kTypeThreeSecondResource = 10000031;
constexpr std::int32_t kTypeThreeThirdResource = 10000032;
constexpr std::int32_t kTypeFourWarningResource = 10000002;
constexpr std::int32_t kTypeFourBurstResource = 10000000;
constexpr std::int32_t kTypeFiveFirstResource = 10000051;
constexpr std::int32_t kTypeFiveSecondResource = 10000050;
constexpr std::int32_t kTypeFiveThirdResource = 10000052;
constexpr std::int32_t kTypeTenResource = 10000060;
constexpr std::int32_t kTypeTwelveWarningResource = 10000080;
constexpr std::int32_t kTypeTwelveProjectileResource = 10000081;
constexpr std::int32_t kTypeFourteenProjectileResource = 10000070;
constexpr std::int32_t kTypeSixteenProjectileResource = 10000110;
constexpr std::int32_t kTypeSixteenExplosionResource = 10000111;
constexpr std::int32_t kTypeTwentyOneSourceResource = 11000210;
constexpr std::int32_t kTypeTwentyOneProjectileResource = 10000100;
constexpr std::array<std::int32_t, 5>
    kTypeTwentyOneStageResources{{
        -1,
        12000000,
        11000033,
        10000030,
        10000060,
    }};
constexpr std::array<std::int32_t, 5>
    kTypeTwentyOneStageEffects{{
        -1,
        20000,
        21013,
        20005,
        21000,
    }};
constexpr std::int32_t kTypeOneAudioSample = 19;
constexpr std::int32_t kTypeTwoAudioSample = 94;
constexpr std::int32_t kTypeThreeAudioSample = 21;
constexpr std::int32_t kTypeFourFirstAudioSample = 29;
constexpr std::int32_t kTypeFourSecondAudioSample = 23;
constexpr std::int32_t kWavePulseAudioSample = 22;
constexpr std::int32_t kChildDistance = 180;
constexpr std::int32_t kChildRadius = 50;
constexpr std::int32_t kTypeThreeFirstRadius = 250;
constexpr std::int32_t kTypeThreeRadiusStep = 200;
constexpr std::int32_t kTypeThreeWavePeriod = 4;
constexpr std::int32_t kTypeThreeRadius = 100;
constexpr std::int32_t kTypeThreeWaveTable = 205;
constexpr std::int32_t kTypeTenWaveTable = 206;
constexpr std::int32_t kTypeTwentyOneCountTable = 207;
constexpr std::int32_t kTypeElevenCountTable = 204;
constexpr std::int32_t kTypeFourWarningUpdate = 3;
constexpr std::int32_t kTypeFourDisplayHeight = 200;
constexpr std::int32_t kAreaDamageExpansion = 150;
constexpr std::int32_t kNearbyShakeRange = 3001;
constexpr std::int32_t kNearbyShakeDuration = 8;
constexpr std::int32_t kNearbyShakeMagnitude = 6;
constexpr std::int32_t kTypeFiveFirstUpdate = 3;
constexpr std::int32_t kTypeFiveDamageOffset = 4;
constexpr std::int32_t kTypeFiveThirdVisualOffset = 15;
constexpr std::int32_t kTypeFiveLifetimeOffset = 22;
constexpr std::int32_t kTypeFivePulsePeriod = 3;
constexpr std::int32_t kTypeFivePulseRemainder = 2;
constexpr std::int32_t kTypeTenFirstRadius = 250;
constexpr std::int32_t kTypeTenRadiusStep = 300;
constexpr std::int32_t kTypeTenWavePeriod = 8;
constexpr std::int32_t kTypeTenRadius = 150;
constexpr std::int32_t kTypeElevenRadius = 80;
constexpr std::int32_t kTypeElevenLifetime = 90;
constexpr std::int32_t kTypeElevenTurnSpeed = 20;
constexpr std::int32_t kTypeTwelveWarningDistance = 150;
constexpr std::int32_t kTypeTwelveProjectileLifetime = 90;
constexpr double kTypeTwelveSpreadRadians = 2.5132736;
constexpr std::int32_t kTypeTwelveFirstImpactEffect = 21021;
constexpr std::int32_t kTypeTwelveSecondImpactEffect = 21022;
constexpr std::int32_t kTypeThirteenFirstRadius = 350;
constexpr std::int32_t kTypeThirteenRadiusStep = 200;
constexpr std::int32_t kTypeThirteenWavePeriod = 4;
constexpr std::int32_t kTypeThirteenLifetime = 16;
constexpr std::int32_t kTypeSixteenProjectileRadius = 80;
constexpr std::int32_t kTypeSixteenExplosionRadius = 240;
constexpr std::int32_t kTypeSixteenExplosionCollisionUpdate = 5;
constexpr std::int32_t kTypeTwentyOneTurnSpeed = 20;
constexpr std::int32_t kTypeTwentyOneAnimationDistance = 30;
constexpr std::int32_t kTypeTwentyOneStagePeriod = 4;
constexpr std::int32_t kTypeTwentyOneProjectileRadius = 80;
constexpr std::int32_t kTypeTwentyOneStageRadius = 240;

bool supportedEffect(std::int32_t effect_number) {
    return effect_number == kTypeOneEffect ||
           effect_number == kTypeTwoEffect ||
           effect_number == kTypeThreeEffect ||
           effect_number == kTypeFourEffect ||
           effect_number == kTypeFiveEffect ||
           effect_number == kTypeTenEffect ||
           effect_number == kTypeElevenEffect ||
           effect_number == kTypeTwelveEffect ||
           effect_number == kTypeThirteenEffect ||
           effect_number == kTypeFourteenEffect ||
           effect_number == kTypeSixteenEffect ||
           effect_number == kTypeTwentyOneEffect;
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
        request.effect_number == kTypeTwoEffect ||
                request.effect_number == kTypeTwelveEffect
            ? kTypeTwoSourceResource
            : kTypeOneSourceResource;
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
    if (request.effect_number == kTypeOneEffect) {
        actor.resource_id = kTypeOneChildResource;
    } else if (
        request.effect_number == kTypeFourteenEffect) {
        actor.resource_id =
            kTypeFourteenProjectileResource;
    } else {
        actor.resource_id = kTypeTwoChildResource;
    }
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

RuntimeEffectActorSpawnRequest typeElevenActor(
    const CombatEffectSpawnRequest& request,
    WorldPosition position,
    double direction_radians) {
    RuntimeEffectActorSpawnRequest actor;
    actor.controller_effect_number =
        request.effect_number;
    actor.resource_id = kTypeOneChildResource;
    actor.owner_kind = request.owner_kind;
    actor.source_character_number =
        request.source_character_number;
    actor.target_mask = request.target_kind;
    actor.target_identifier =
        request.target_identifier;
    actor.home_toward_target = true;
    actor.homing_turn_speed =
        kTypeElevenTurnSpeed;
    actor.direction_radians = direction_radians;
    actor.travel_speed =
        request.constructor_value_6;
    actor.position = position;
    actor.judgement = {
        -kTypeElevenRadius,
        -kTypeElevenRadius,
        kTypeElevenRadius - 1,
        kTypeElevenRadius - 1,
    };
    actor.display_height =
        request.constructor_value_7;
    actor.lifetime = kTypeElevenLifetime;
    actor.expire_on_environment_collision = true;
    actor.target_collision_start = 0;
    actor.expire_on_target = true;
    actor.target_audio = {0, 20};
    actor.animation_chart = 0;
    actor.animation_direction =
        retailDirectionForAngle(direction_radians);
    actor.has_packet = request.has_packet;
    actor.packet = request.packet;
    return actor;
}

double typeTwelveDirection(
    const CombatEffectSpawnRequest& request,
    std::int32_t actor_count,
    std::int32_t spread_divisor,
    std::int32_t index) {
    if (actor_count == 1) {
        return request.direction_radians;
    }
    const double spread =
        static_cast<double>(actor_count) *
        kTypeTwelveSpreadRadians /
        static_cast<double>(spread_divisor);
    const std::int32_t even =
        1 - (actor_count & 1);
    return request.direction_radians -
           spread * 0.5 +
           (spread /
            static_cast<double>(actor_count)) *
               static_cast<double>(even) *
               0.5 +
           static_cast<double>(index) * spread /
               static_cast<double>(actor_count - 1);
}

RuntimeEffectActorSpawnRequest typeTwelveWarningActor(
    const CombatEffectSpawnRequest& request,
    WorldPosition position,
    double direction_radians) {
    RuntimeEffectActorSpawnRequest actor;
    actor.controller_effect_number =
        request.effect_number;
    actor.resource_id = kTypeTwelveWarningResource;
    actor.owner_kind = request.owner_kind;
    actor.source_character_number =
        request.source_character_number;
    actor.target_mask = request.target_kind;
    actor.target_identifier =
        request.target_identifier;
    actor.direction_radians = direction_radians;
    actor.position = position;
    actor.judgement = {
        -kChildRadius,
        -kChildRadius,
        kChildRadius,
        kChildRadius,
    };
    actor.display_height =
        request.constructor_value_7;
    actor.lifetime =
        request.constructor_value_17;
    actor.animation_chart = 0;
    actor.animation_direction =
        retailDirectionForAngle(direction_radians);
    actor.has_packet = request.has_packet;
    actor.packet = request.packet;
    return actor;
}

RuntimeEffectActorSpawnRequest typeTwelveProjectileActor(
    const CombatEffectSpawnRequest& request,
    WorldPosition position,
    double direction_radians) {
    RuntimeEffectActorSpawnRequest actor;
    actor.controller_effect_number =
        request.effect_number;
    actor.resource_id = kTypeTwelveProjectileResource;
    actor.owner_kind = request.owner_kind;
    actor.source_character_number =
        request.source_character_number;
    actor.target_mask = request.target_kind;
    actor.target_identifier =
        request.target_identifier;
    actor.direction_radians = direction_radians;
    actor.travel_speed =
        request.constructor_value_6;
    actor.position = position;
    actor.judgement = {
        -kChildRadius,
        -kChildRadius,
        kChildRadius,
        kChildRadius,
    };
    actor.display_height =
        request.constructor_value_7;
    actor.lifetime = kTypeTwelveProjectileLifetime;
    actor.expire_on_environment_collision = true;
    actor.target_collision_start = 0;
    actor.expire_on_target = true;
    actor.remember_targets =
        request.constructor_value_22 == 1;
    actor.target_audio = {0, 20};
    actor.animation_chart = 0;
    actor.animation_direction =
        retailDirectionForAngle(direction_radians);
    actor.has_packet = request.has_packet;
    actor.packet = request.packet;
    actor.packet.write(
        34, kTypeTwelveFirstImpactEffect);
    actor.packet.write(
        35, actor.animation_direction);
    actor.packet.write(
        74, kTypeTwelveSecondImpactEffect);
    actor.packet.write(
        75, actor.animation_direction);
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

RuntimeEffectActorSpawnRequest typeSixteenProjectileActor(
    const CombatEffectSpawnRequest& request,
    WorldPosition position) {
    RuntimeEffectActorSpawnRequest actor;
    actor.controller_effect_number =
        request.effect_number;
    actor.resource_id =
        kTypeSixteenProjectileResource;
    actor.owner_kind = request.owner_kind;
    actor.source_character_number =
        request.source_character_number;
    actor.target_mask = request.target_kind;
    actor.target_identifier =
        request.target_identifier;
    actor.direction_radians =
        request.direction_radians;
    actor.travel_speed =
        request.constructor_value_6;
    actor.position = position;
    actor.judgement = {
        -kTypeSixteenProjectileRadius,
        -kTypeSixteenProjectileRadius,
        kTypeSixteenProjectileRadius - 1,
        kTypeSixteenProjectileRadius - 1,
    };
    actor.display_height =
        request.constructor_value_7;
    actor.expire_on_environment_collision = true;
    actor.target_collision_start = 0;
    actor.expire_on_target = true;
    actor.remember_targets =
        request.constructor_value_22 == 1;
    actor.target_audio = {0, 20};
    actor.animation_chart = 0;
    actor.animation_direction =
        retailDirectionForAngle(
            request.direction_radians);
    actor.track_for_controller = true;
    actor.has_packet = request.has_packet;
    actor.packet = request.packet;
    return actor;
}

RuntimeEffectActorSpawnRequest typeSixteenExplosionActor(
    const CombatEffectSpawnRequest& request,
    WorldPosition position) {
    RuntimeEffectActorSpawnRequest actor;
    actor.controller_effect_number =
        request.effect_number;
    actor.resource_id =
        kTypeSixteenExplosionResource;
    actor.owner_kind = request.owner_kind;
    actor.source_character_number =
        request.source_character_number;
    actor.target_mask = request.target_kind;
    actor.target_identifier =
        request.target_identifier;
    actor.position = position;
    actor.judgement = {
        -kTypeSixteenExplosionRadius,
        -kTypeSixteenExplosionRadius,
        kTypeSixteenExplosionRadius - 1,
        kTypeSixteenExplosionRadius - 1,
    };
    actor.lifetime_from_animation = true;
    actor.target_collision_start =
        kTypeSixteenExplosionCollisionUpdate;
    actor.target_collision_end =
        kTypeSixteenExplosionCollisionUpdate;
    actor.process_every_target = true;
    actor.target_audio = {0, 20};
    actor.animation_chart = 0;
    actor.animation_direction = 8;
    actor.has_packet = request.has_packet;
    actor.packet = request.packet;
    return actor;
}

RuntimeEffectActorSpawnRequest typeTwentyOneSourceActor(
    const CombatEffectSpawnRequest& request,
    WorldPosition position) {
    RuntimeEffectActorSpawnRequest actor;
    actor.controller_effect_number =
        request.effect_number;
    actor.resource_id =
        kTypeTwentyOneSourceResource;
    actor.owner_kind = request.owner_kind;
    actor.source_character_number =
        request.source_character_number;
    actor.position = position;
    const ObjectBounds source =
        request.has_source_judgement
            ? request.source_judgement
            : ObjectBounds{};
    const std::int32_t x =
        retailAdd(source.right, 1);
    const std::int32_t y =
        retailAdd(source.bottom, 1);
    actor.judgement = {x, y, x, y};
    actor.lifetime_from_animation = true;
    actor.animation_chart = 0;
    actor.animation_direction = 8;
    return actor;
}

RuntimeEffectActorSpawnRequest typeTwentyOneProjectileActor(
    const CombatEffectSpawnRequest& request,
    WorldPosition position,
    double direction_radians,
    std::int32_t tracking_index) {
    RuntimeEffectActorSpawnRequest actor;
    actor.controller_effect_number =
        request.effect_number;
    actor.resource_id =
        kTypeTwentyOneProjectileResource;
    actor.owner_kind = request.owner_kind;
    actor.source_character_number =
        request.source_character_number;
    actor.target_mask = request.target_kind;
    actor.target_identifier =
        request.target_identifier;
    actor.home_toward_target = true;
    actor.homing_turn_speed =
        kTypeTwentyOneTurnSpeed;
    actor.direction_radians = direction_radians;
    actor.travel_speed =
        request.constructor_value_6;
    actor.position = position;
    actor.judgement = {
        -kTypeTwentyOneProjectileRadius,
        -kTypeTwentyOneProjectileRadius,
        kTypeTwentyOneProjectileRadius - 1,
        kTypeTwentyOneProjectileRadius - 1,
    };
    actor.display_height =
        request.constructor_value_7;
    actor.lifetime_from_animation = true;
    actor.expire_on_environment_collision = true;
    actor.target_collision_start = 0;
    actor.expire_on_target = true;
    actor.target_audio = {0, 20};
    actor.animation_chart = 0;
    actor.animation_direction = 8;
    actor.animation_speed =
        retailMultiply(
            request.constructor_value_6, 1000) /
        kTypeTwentyOneAnimationDistance;
    actor.track_for_controller = true;
    actor.controller_tracking_index =
        tracking_index;
    actor.has_packet = request.has_packet;
    actor.packet = request.packet;
    return actor;
}

RuntimeEffectActorSpawnRequest typeTwentyOneStageActor(
    const CombatEffectSpawnRequest& request,
    WorldPosition position,
    std::int32_t stage) {
    RuntimeEffectActorSpawnRequest actor;
    actor.controller_effect_number =
        request.effect_number;
    actor.resource_id =
        kTypeTwentyOneStageResources[
            static_cast<std::size_t>(stage)];
    actor.owner_kind = request.owner_kind;
    actor.source_character_number =
        request.source_character_number;
    actor.target_mask = request.target_kind;
    actor.target_identifier =
        request.target_identifier;
    actor.direction_radians =
        request.direction_radians;
    actor.position = position;
    actor.judgement = {
        -kTypeTwentyOneStageRadius,
        -kTypeTwentyOneStageRadius,
        kTypeTwentyOneStageRadius - 1,
        kTypeTwentyOneStageRadius - 1,
    };
    actor.lifetime_from_animation = true;
    if ((stage & 1) == 0) {
        actor.target_collision_start = 0;
        actor.target_collision_end = 0;
    }
    actor.process_every_target = true;
    actor.target_audio = {0, 20};
    actor.animation_chart = 0;
    actor.animation_direction = 8;
    actor.has_packet = request.has_packet;
    actor.packet = request.packet;
    actor.packet.write(32, stage - 1);
    actor.packet.write(
        34,
        kTypeTwentyOneStageEffects[
            static_cast<std::size_t>(stage)]);
    return actor;
}

RuntimeEffectActorSpawnRequest stationaryVisual(
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

RuntimeEffectActorSpawnRequest oneUpdateAreaDamageActor(
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
            kAreaDamageExpansion),
        retailSubtract(
            source.top,
            kAreaDamageExpansion),
        retailAdd(
            source.right,
            kAreaDamageExpansion),
        retailAdd(
            source.bottom,
            kAreaDamageExpansion),
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

RuntimeEffectActorSpawnRequest typeTenActor(
    const CombatEffectSpawnRequest& request,
    WorldPosition position) {
    RuntimeEffectActorSpawnRequest actor;
    actor.controller_effect_number =
        request.effect_number;
    actor.resource_id = kTypeTenResource;
    actor.owner_kind = request.owner_kind;
    actor.source_character_number =
        request.source_character_number;
    actor.target_mask = request.target_kind;
    actor.target_identifier = 0;
    actor.position = position;
    actor.judgement = {
        -kTypeTenRadius,
        -kTypeTenRadius,
        kTypeTenRadius,
        kTypeTenRadius,
    };
    actor.lifetime_from_animation = true;
    actor.target_collision_start = 0;
    actor.target_collision_end = 0;
    actor.process_every_target = true;
    actor.animation_chart = 0;
    actor.animation_direction = 8;
    actor.has_packet = request.has_packet;
    actor.packet = request.packet;
    return actor;
}

ObjectBounds sourcePointJudgement(
    const CombatEffectSpawnRequest& request) {
    const ObjectBounds source =
        request.has_source_judgement
            ? request.source_judgement
            : ObjectBounds{};
    const std::int32_t x =
        retailAdd(source.right, 1);
    const std::int32_t y =
        retailAdd(source.bottom, 1);
    return {x, y, x, y};
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
    type_twenty_one_actor_identifiers_.fill(-1);
    if (!request.valid ||
        !supportedEffect(request.effect_number)) {
        return false;
    }
    if (request.effect_number == kTypeThreeEffect ||
        request.effect_number == kTypeTenEffect) {
        const TableData* wave_table =
            tables
                ? tables->find(
                      request.effect_number ==
                              kTypeThreeEffect
                          ? kTypeThreeWaveTable
                          : kTypeTenWaveTable)
                : nullptr;
        const std::int32_t column =
            retailSubtract(
                request.constructor_value_17, 1);
        if (!wave_table ||
            !wave_table->contains(0, column)) {
            return false;
        }
        wave_count_ =
            wave_table->value(0, column);
    }
    if (request.effect_number == kTypeElevenEffect ||
        request.effect_number == kTypeTwelveEffect ||
        request.effect_number == kTypeThirteenEffect) {
        const TableData* count_table =
            tables
                ? tables->find(
                      kTypeElevenCountTable)
                : nullptr;
        const std::int32_t column =
            retailSubtract(
                request.constructor_value_17, 1);
        if (!count_table ||
            !count_table->contains(0, column)) {
            return false;
        }
        radial_actor_count_ =
            count_table->value(0, column);
        if (radial_actor_count_ < 0 ||
            static_cast<std::size_t>(
                radial_actor_count_) >
                radial_placement_blocked_.size()) {
            return false;
        }
        if (request.effect_number == kTypeTwelveEffect) {
            if (!count_table->contains(0, 29)) {
                return false;
            }
            radial_spread_divisor_ =
                count_table->value(0, 29);
            if (radial_spread_divisor_ == 0) {
                return false;
            }
        }
    }
    if (request.effect_number ==
        kTypeTwentyOneEffect) {
        const TableData* count_table =
            tables
                ? tables->find(
                      kTypeTwentyOneCountTable)
                : nullptr;
        const std::int32_t column =
            retailSubtract(
                request.constructor_value_17, 1);
        if (!count_table ||
            !count_table->contains(0, column)) {
            return false;
        }
        type_twenty_one_actor_count_ =
            count_table->value(0, column);
        if (type_twenty_one_actor_count_ < 0 ||
            static_cast<std::size_t>(
                type_twenty_one_actor_count_) >
                type_twenty_one_stages_.size()) {
            return false;
        }
    }
    request_ = request;
    type_five_position_ =
        request.has_explicit_origin
            ? request.origin
            : WorldPosition{};
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
                        wave_index_,
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
                wave_placement_blocked_ = true;
            }
            if (!wave_placement_blocked_) {
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
            wave_index_ =
                retailAdd(wave_index_, 1);
        }

        counter_ = retailAdd(counter_, 1);
        if (counter_ ==
            retailAdd(
                request_.constructor_value_12,
                retailMultiply(
                    wave_count_,
                    kTypeThreeWavePeriod))) {
            active_ = false;
            result.expired = true;
        }
        return result;
    }

    if (request_.effect_number == kTypeTenEffect) {
        if (request_.constructor_value_12 <= counter_ &&
            retailSubtract(
                counter_,
                request_.constructor_value_12) %
                    kTypeTenWavePeriod ==
                0) {
            const std::int32_t distance =
                retailAdd(
                    retailMultiply(
                        wave_index_,
                        kTypeTenRadiusStep),
                    kTypeTenFirstRadius);
            const WorldPosition position =
                projectedPosition(
                    request_.has_explicit_origin
                        ? request_.origin
                        : WorldPosition{},
                    request_.direction_radians,
                    distance);
            const ObjectBounds judgement{
                -kTypeTenRadius,
                -kTypeTenRadius,
                kTypeTenRadius,
                kTypeTenRadius,
            };
            if (context.placement_is_clear &&
                !context.placement_is_clear(
                    position, judgement)) {
                wave_placement_blocked_ = true;
            }
            if (!wave_placement_blocked_) {
                result.actor_spawns[
                    result.actor_spawn_count++] =
                    typeTenActor(
                        request_, position);
                result.audio[result.audio_count++] = {
                    kWavePulseAudioSample,
                    position,
                };
                if (context.observer.found &&
                    positionDistance(
                        context.observer.position,
                        position) <
                        kNearbyShakeRange) {
                    result.camera_shake = true;
                    result.camera_shake_duration =
                        kNearbyShakeDuration;
                    result.camera_shake_magnitude =
                        kNearbyShakeMagnitude;
                }
            }
            wave_index_ =
                retailAdd(wave_index_, 1);
        }

        counter_ = retailAdd(counter_, 1);
        if (counter_ ==
            retailAdd(
                request_.constructor_value_12,
                retailMultiply(
                    wave_count_,
                    kTypeTenWavePeriod))) {
            active_ = false;
            result.expired = true;
        }
        return result;
    }

    if (request_.effect_number == kTypeThirteenEffect) {
        if (request_.constructor_value_12 <= counter_ &&
            retailSubtract(
                counter_,
                request_.constructor_value_12) %
                    kTypeThirteenWavePeriod ==
                0) {
            const double angle_step =
                radial_actor_count_ > 0
                    ? kRetailFullCircleRadians /
                          static_cast<double>(
                              radial_actor_count_)
                    : 0.0;
            WorldPosition audio_position =
                request_.has_explicit_origin
                    ? request_.origin
                    : WorldPosition{};
            for (std::int32_t index = 0;
                 index < radial_actor_count_;
                 ++index) {
                const double direction =
                    request_.direction_radians +
                    static_cast<double>(index) *
                        angle_step;
                const std::int32_t distance =
                    retailAdd(
                        retailMultiply(
                            wave_index_,
                            kTypeThirteenRadiusStep),
                        kTypeThirteenFirstRadius);
                const WorldPosition position =
                    projectedPosition(
                        request_.has_explicit_origin
                            ? request_.origin
                            : WorldPosition{},
                        direction,
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
                    radial_placement_blocked_[
                        static_cast<std::size_t>(index)] =
                        true;
                }
                if (!radial_placement_blocked_[
                        static_cast<std::size_t>(index)]) {
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
                }
                audio_position = position;
            }
            result.audio[result.audio_count++] = {
                kTypeThreeAudioSample,
                audio_position,
            };
            wave_index_ =
                retailAdd(wave_index_, 1);
        }

        counter_ = retailAdd(counter_, 1);
        if (counter_ ==
            retailAdd(
                request_.constructor_value_12,
                kTypeThirteenLifetime)) {
            active_ = false;
            result.expired = true;
        }
        return result;
    }

    if (request_.effect_number ==
        kTypeTwentyOneEffect) {
        if (counter_ == 0) {
            result.actor_spawns[
                result.actor_spawn_count++] =
                typeTwentyOneSourceActor(
                    request_,
                    resolvedPosition(
                        request_, context.source));
        }

        if (counter_ == request_.constructor_value_12) {
            const double angle_step =
                type_twenty_one_actor_count_ > 0
                    ? kRetailFullCircleRadians /
                          static_cast<double>(
                              type_twenty_one_actor_count_)
                    : 0.0;
            WorldPosition audio_position =
                resolvedPosition(
                    request_, context.source);
            for (std::int32_t index = 0;
                 index < type_twenty_one_actor_count_;
                 ++index) {
                const double direction =
                    request_.direction_radians -
                    static_cast<double>(index) *
                        angle_step;
                const WorldPosition source =
                    resolvedPosition(
                        request_, context.source);
                const WorldPosition position =
                    request_.owner_kind == 0
                        ? source
                        : projectedPosition(
                              source,
                              direction,
                              kChildDistance);
                result.actor_spawns[
                    result.actor_spawn_count++] =
                    typeTwentyOneProjectileActor(
                        request_,
                        position,
                        direction,
                        index);
                type_twenty_one_actor_positions_[
                    static_cast<std::size_t>(index)] =
                    position;
                audio_position = position;
            }
            if (type_twenty_one_actor_count_ > 0) {
                result.audio[result.audio_count++] = {
                    kTypeOneAudioSample,
                    audio_position,
                };
            }
        }

        if (request_.constructor_value_12 < counter_) {
            for (std::int32_t index = 0;
                 index < type_twenty_one_actor_count_;
                 ++index) {
                const std::size_t actor_index =
                    static_cast<std::size_t>(index);
                const EnemyEffectControllerSource actor =
                    context.resolve_actor
                        ? context.resolve_actor(
                              type_twenty_one_actor_identifiers_[
                                  actor_index])
                        : EnemyEffectControllerSource{};
                if (actor.found) {
                    type_twenty_one_actor_positions_[
                        actor_index] = actor.position;
                } else if (
                    type_twenty_one_stages_[
                        actor_index] == 0) {
                    type_twenty_one_stages_[
                        actor_index] = 1;
                }
            }
        }

        for (std::int32_t index = 0;
             index < type_twenty_one_actor_count_;
             ++index) {
            const std::size_t actor_index =
                static_cast<std::size_t>(index);
            std::int32_t& stage =
                type_twenty_one_stages_[actor_index];
            if (stage == 0) {
                continue;
            }
            std::int32_t& stage_counter =
                type_twenty_one_stage_counters_[
                    actor_index];
            if (stage_counter %
                        kTypeTwentyOneStagePeriod ==
                    0 &&
                stage != 5) {
                const WorldPosition position =
                    type_twenty_one_actor_positions_[
                        actor_index];
                RuntimeEffectActorSpawnRequest stage_actor =
                    typeTwentyOneStageActor(
                        request_, position, stage);
                result.actor_spawns[
                    result.actor_spawn_count++] =
                    stage_actor;
                if (stage == 3) {
                    stage_actor.target_collision_start = -1;
                    stage_actor.target_collision_end = -1;
                    stage_actor.process_every_target = false;
                    stage_actor.resource_id =
                        kTypeThreeSecondResource;
                    result.actor_spawns[
                        result.actor_spawn_count++] =
                        stage_actor;
                    stage_actor.resource_id =
                        kTypeThreeThirdResource;
                    result.actor_spawns[
                        result.actor_spawn_count++] =
                        stage_actor;
                }
                if (stage == 4) {
                    if (context.observer.found &&
                        positionDistance(
                            context.observer.position,
                            position) <
                            kNearbyShakeRange) {
                        result.camera_shake = true;
                        result.camera_shake_duration =
                            kNearbyShakeDuration;
                        result.camera_shake_magnitude =
                            kNearbyShakeMagnitude;
                    }
                    result.audio[
                        result.audio_count++] = {
                        kWavePulseAudioSample,
                        position,
                    };
                }
                result.audio[result.audio_count++] = {
                    kTypeOneAudioSample,
                    position,
                };
                stage = retailAdd(stage, 1);
            }
            stage_counter =
                retailAdd(stage_counter, 1);
        }

        bool complete = true;
        for (std::int32_t index = 0;
             index < type_twenty_one_actor_count_;
             ++index) {
            if (type_twenty_one_stages_[
                    static_cast<std::size_t>(index)] != 5) {
                complete = false;
                break;
            }
        }
        if (complete) {
            active_ = false;
            result.expired = true;
            return result;
        }

        counter_ = retailAdd(counter_, 1);
        return result;
    }

    if (request_.effect_number == kTypeSixteenEffect) {
        if (counter_ == request_.constructor_value_12) {
            const WorldPosition source =
                resolvedPosition(
                    request_, context.source);
            const WorldPosition position =
                request_.owner_kind == 0
                    ? source
                    : projectedPosition(
                          source,
                          request_.direction_radians,
                          kChildDistance);
            result.actor_spawns[
                result.actor_spawn_count++] =
                typeSixteenProjectileActor(
                    request_, position);
            result.audio[result.audio_count++] = {
                kTypeOneAudioSample,
                position,
            };
        } else if (
            request_.constructor_value_12 < counter_) {
            const EnemyEffectControllerSource actor =
                context.resolve_actor
                    ? context.resolve_actor(
                          tracked_actor_identifier_)
                    : EnemyEffectControllerSource{};
            if (actor.found) {
                tracked_actor_position_ =
                    actor.position;
            } else {
                result.actor_spawns[
                    result.actor_spawn_count++] =
                    typeSixteenExplosionActor(
                        request_,
                        tracked_actor_position_);
                result.audio[result.audio_count++] = {
                    kWavePulseAudioSample,
                    tracked_actor_position_,
                };
                if (context.observer.found &&
                    positionDistance(
                        context.observer.position,
                        tracked_actor_position_) <
                        kNearbyShakeRange) {
                    result.camera_shake = true;
                    result.camera_shake_duration =
                        kNearbyShakeDuration;
                    result.camera_shake_magnitude =
                        kNearbyShakeMagnitude;
                }
                active_ = false;
                result.expired = true;
                return result;
            }
        }

        counter_ = retailAdd(counter_, 1);
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
                stationaryVisual(
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
                stationaryVisual(
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
                stationaryVisual(
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
                    kNearbyShakeRange) {
                result.camera_shake = true;
                result.camera_shake_duration =
                    kNearbyShakeDuration;
                result.camera_shake_magnitude =
                    kNearbyShakeMagnitude;
            }
            result.actor_spawns[
                result.actor_spawn_count++] =
                oneUpdateAreaDamageActor(
                    request_, position);
            active_ = false;
            result.expired = true;
            return result;
        }

        counter_ = retailAdd(counter_, 1);
        return result;
    }

    if (request_.effect_number == kTypeFiveEffect) {
        if (counter_ == kTypeFiveFirstUpdate) {
            type_five_position_ =
                resolvedPosition(
                    request_, context.source);
            result.actor_spawns[
                result.actor_spawn_count++] =
                stationaryVisual(
                    request_,
                    type_five_position_,
                    sourcePointJudgement(request_),
                    kTypeFiveFirstResource,
                    0,
                    0,
                    0,
                    0);
        }

        const std::int32_t first_length =
            context.resolve_animation_length
                ? context.resolve_animation_length(
                      kTypeFiveFirstResource, 0, 8)
                : 0;
        if (first_length < 1) {
            return result;
        }
        if (counter_ == first_length) {
            result.actor_spawns[
                result.actor_spawn_count++] =
                stationaryVisual(
                    request_,
                    type_five_position_,
                    sourcePointJudgement(request_),
                    kTypeFiveSecondResource,
                    0,
                    0,
                    kTypeFourDisplayHeight,
                    0);
        }
        if (counter_ ==
            retailAdd(
                first_length,
                kTypeFiveDamageOffset)) {
            if (context.observer.found &&
                positionDistance(
                    context.observer.position,
                    type_five_position_) <
                    kNearbyShakeRange) {
                result.camera_shake = true;
                result.camera_shake_duration =
                    kNearbyShakeDuration;
                result.camera_shake_magnitude =
                    kNearbyShakeMagnitude;
            }
            result.actor_spawns[
                result.actor_spawn_count++] =
                oneUpdateAreaDamageActor(
                    request_, type_five_position_);
        }
        if (counter_ ==
            retailAdd(
                first_length,
                kTypeFiveThirdVisualOffset)) {
            result.actor_spawns[
                result.actor_spawn_count++] =
                stationaryVisual(
                    request_,
                    type_five_position_,
                    sourcePointJudgement(request_),
                    kTypeFiveThirdResource,
                    0,
                    0,
                    kTypeFourDisplayHeight,
                    0x80);
        }

        const std::int32_t phase =
            retailSubtract(counter_, first_length);
        if (phase >= kTypeFiveDamageOffset &&
            phase <= kTypeFiveLifetimeOffset &&
            retailSubtract(
                phase,
                kTypeFiveDamageOffset) %
                    kTypeFivePulsePeriod ==
                kTypeFivePulseRemainder) {
            result.audio[result.audio_count++] = {
                kWavePulseAudioSample,
                type_five_position_,
            };
        }

        counter_ = retailAdd(counter_, 1);
        if (counter_ ==
            retailAdd(
                first_length,
                kTypeFiveLifetimeOffset)) {
            active_ = false;
            result.expired = true;
        }
        return result;
    }

    if (counter_ == 0 &&
        request_.effect_number != kTypeFourteenEffect) {
        result.actor_spawns[
            result.actor_spawn_count++] =
            sourceActor(
                request_,
                resolvedPosition(
                    request_, context.source));
        if (request_.effect_number ==
            kTypeTwelveEffect) {
            const WorldPosition source =
                resolvedPosition(
                    request_, context.source);
            for (std::int32_t index = 0;
                 index < radial_actor_count_;
                 ++index) {
                const double direction =
                    typeTwelveDirection(
                        request_,
                        radial_actor_count_,
                        radial_spread_divisor_,
                        index);
                const WorldPosition position =
                    request_.owner_kind == 0
                        ? source
                        : projectedPosition(
                              source,
                              direction,
                              kTypeTwelveWarningDistance);
                result.actor_spawns[
                    result.actor_spawn_count++] =
                    typeTwelveWarningActor(
                        request_,
                        position,
                        direction);
            }
        }
    }

    if (request_.effect_number ==
            kTypeElevenEffect &&
        counter_ == request_.constructor_value_12) {
        const WorldPosition source =
            resolvedPosition(
                request_, context.source);
        WorldPosition audio_position = source;
        const double angle_step =
            radial_actor_count_ > 0
                ? kRetailFullCircleRadians /
                      static_cast<double>(
                          radial_actor_count_)
                : 0.0;
        for (std::int32_t index = 0;
             index < radial_actor_count_;
             ++index) {
            const double direction =
                request_.direction_radians -
                static_cast<double>(index) *
                    angle_step;
            const WorldPosition position =
                request_.owner_kind == 0
                    ? source
                    : projectedPosition(
                          source,
                          direction,
                          kChildDistance);
            result.actor_spawns[
                result.actor_spawn_count++] =
                typeElevenActor(
                    request_,
                    position,
                    direction);
            audio_position = position;
        }
        result.audio[result.audio_count++] = {
            kTypeOneAudioSample,
            audio_position,
        };
        active_ = false;
        result.expired = true;
        return result;
    }

    if (request_.effect_number ==
            kTypeTwelveEffect &&
        counter_ == request_.constructor_value_12) {
        const WorldPosition source =
            resolvedPosition(
                request_, context.source);
        WorldPosition audio_position = source;
        for (std::int32_t index = 0;
             index < radial_actor_count_;
             ++index) {
            const double direction =
                typeTwelveDirection(
                    request_,
                    radial_actor_count_,
                    radial_spread_divisor_,
                    index);
            const WorldPosition position =
                request_.owner_kind == 0
                    ? source
                    : projectedPosition(
                          source,
                          direction,
                          kChildDistance);
            result.actor_spawns[
                result.actor_spawn_count++] =
                typeTwelveProjectileActor(
                    request_,
                    position,
                    direction);
            audio_position = position;
        }
        result.audio[result.audio_count++] = {
            kTypeTwoAudioSample,
            audio_position,
        };
        active_ = false;
        result.expired = true;
        return result;
    }

    if (counter_ == request_.constructor_value_12) {
        const WorldPosition source =
            resolvedPosition(
                request_, context.source);
        const WorldPosition position =
            request_.owner_kind == 0
                ? source
                : projectedPosition(
                      source,
                      request_.direction_radians,
                      kChildDistance);
        result.actor_spawns[
            result.actor_spawn_count++] =
            childActor(request_, position);
        result.audio[result.audio_count++] = {
            request_.effect_number == kTypeOneEffect
                ? kTypeOneAudioSample
                : request_.effect_number ==
                          kTypeFourteenEffect
                      ? kWavePulseAudioSample
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

void EnemyEffectController::bindSpawnedActor(
    std::int32_t actor_identifier,
    const EnemyEffectControllerSource& actor,
    std::int32_t tracking_index) {
    if (!active_) {
        return;
    }
    if (request_.effect_number == kTypeSixteenEffect) {
        tracked_actor_identifier_ = actor_identifier;
        if (actor.found) {
            tracked_actor_position_ = actor.position;
        }
        return;
    }
    if (request_.effect_number !=
            kTypeTwentyOneEffect ||
        tracking_index < 0 ||
        tracking_index >=
            type_twenty_one_actor_count_) {
        return;
    }
    const std::size_t index =
        static_cast<std::size_t>(tracking_index);
    type_twenty_one_actor_identifiers_[index] =
        actor_identifier;
    if (actor.found) {
        type_twenty_one_actor_positions_[index] =
            actor.position;
    }
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
