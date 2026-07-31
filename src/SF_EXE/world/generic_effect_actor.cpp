#include "generic_effect_actor.hpp"

#include "actor_direction.hpp"
#include "core/retail_integer.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace osf {
namespace {

constexpr std::array<std::int32_t, 8>
    kEffectResources{{0, 1, 2, 3, 0, 0, 4, 0}};

WorldPosition projectedPosition(
    WorldPosition position,
    double direction,
    std::int32_t distance) {
    return {
        retailAdd(
            position.x,
            static_cast<std::int32_t>(
                std::cos(direction) * distance)),
        retailSubtract(
            position.y,
            static_cast<std::int32_t>(
                std::sin(direction) * distance)),
    };
}

bool commonProjectileEffect(std::int32_t effect_number) {
    return effect_number == 0 ||
           effect_number == 1 ||
           effect_number == 3 ||
           effect_number == 4 ||
           effect_number == 5 ||
           effect_number == 7;
}

std::int32_t effectResource(std::int32_t effect_number) {
    if (effect_number == 10015) {
        return 10000090;
    }
    if (effect_number < 0 ||
        static_cast<std::size_t>(effect_number) >=
            kEffectResources.size()) {
        return -1;
    }
    return kEffectResources[
        static_cast<std::size_t>(effect_number)];
}

}  // namespace

bool buildGenericEffectActor(
    const CombatEffectSpawnRequest& request,
    WorldPosition resolved_source,
    RuntimeEffectActorSpawnRequest& actor) {
    actor = {};
    const bool sonic_blade =
        request.effect_number == 10015;
    if (!request.valid ||
        (!commonProjectileEffect(request.effect_number) &&
         !sonic_blade)) {
        return false;
    }

    actor.controller_effect_number =
        request.effect_number;
    actor.resource_id = effectResource(request.effect_number);
    if (actor.resource_id < 0) {
        return false;
    }
    actor.owner_kind = request.owner_kind;
    actor.source_character_number =
        request.source_character_number;
    actor.target_mask = request.target_kind;
    actor.target_identifier =
        request.target_identifier;
    actor.home_toward_target =
        request.constructor_value_20 == 1 &&
        request.effect_number != 1;
    actor.homing_turn_speed =
        actor.home_toward_target ? 360 : 0;
    actor.direction_radians =
        request.direction_radians;
    actor.travel_speed =
        request.constructor_value_6;
    const WorldPosition origin =
        request.owner_kind == 0 &&
                request.has_explicit_origin
            ? request.origin
            : resolved_source;
    actor.position =
        request.owner_kind == 0
            ? origin
            : projectedPosition(
                  origin,
                  request.direction_radians,
                  request.constructor_value_21);
    // FUN_0042a300 gives effect 10015 its own geometry and fixed clock;
    // the other supported effects use the common projectile descriptor.
    actor.judgement =
        sonic_blade
            ? ObjectBounds{-80, -80, 79, 79}
            : ObjectBounds{-30, -30, 30, 30};
    actor.display_height =
        sonic_blade
            ? 155
            : request.constructor_value_7;
    actor.lifetime = sonic_blade ? 7 : -1;
    actor.collide_with_environment = true;
    actor.expire_on_environment_collision = true;
    actor.target_collision_start = 0;
    actor.expire_on_target = true;
    actor.remember_targets =
        request.constructor_value_22 == 1;
    actor.target_audio = {0, 20};
    actor.animation_chart = 0;
    actor.animation_direction =
        request.effect_number == 1
            ? 8
            : request.packet_kind == 8
                ? retailDirectionForAngle(
                      request.direction_radians)
                : request.packet_kind;
    actor.has_packet = request.has_packet;
    actor.packet = request.packet;
    return true;
}

}  // namespace osf
