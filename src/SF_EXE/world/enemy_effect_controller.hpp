#ifndef OPENSHADOWFLARE_ENEMY_EFFECT_CONTROLLER_HPP
#define OPENSHADOWFLARE_ENEMY_EFFECT_CONTROLLER_HPP

#include "combat_effect_request.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "runtime_effect_actor.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace osf {

class RetailRandom;
class TableDatabase;

struct PositionalEffectAudioRequest {
    std::int32_t sample = -1;
    WorldPosition position;
};

struct EnemyEffectControllerSource {
    bool found = false;
    WorldPosition position;
};

struct EnemyEffectControllerUpdate {
    std::array<RuntimeEffectActorSpawnRequest, 32>
        actor_spawns;
    std::size_t actor_spawn_count = 0;
    std::array<PositionalEffectAudioRequest, 2>
        audio;
    std::size_t audio_count = 0;
    bool camera_shake = false;
    std::int32_t camera_shake_duration = 0;
    std::int32_t camera_shake_magnitude = 0;
    bool expired = false;
};

using EnemyEffectPlacementTest =
    std::function<bool(
        WorldPosition position,
        const ObjectBounds& judgement)>;
using EnemyEffectAnimationLengthResolver =
    std::function<std::int32_t(
        std::int32_t resource_id,
        std::int32_t chart,
        std::int32_t direction)>;
using EnemyEffectActorResolver =
    std::function<EnemyEffectControllerSource(
        std::int32_t actor_identifier)>;

struct EnemyEffectControllerContext {
    EnemyEffectControllerContext(
        EnemyEffectControllerSource source_value = {},
        RetailRandom* random_value = nullptr,
        EnemyEffectPlacementTest placement_value = {},
        EnemyEffectControllerSource observer_value = {},
        EnemyEffectAnimationLengthResolver
            animation_length_value = {},
        EnemyEffectActorResolver actor_value = {})
        : source(source_value),
          random(random_value),
          placement_is_clear(placement_value),
          observer(observer_value),
          resolve_animation_length(animation_length_value),
          resolve_actor(actor_value) {}

    EnemyEffectControllerSource source;
    RetailRandom* random = nullptr;
    EnemyEffectPlacementTest placement_is_clear;
    EnemyEffectControllerSource observer;
    EnemyEffectAnimationLengthResolver
        resolve_animation_length;
    EnemyEffectActorResolver resolve_actor;
};

class EnemyEffectController {
public:
    bool initialize(
        const CombatEffectSpawnRequest& request,
        const TableDatabase* tables = nullptr);
    EnemyEffectControllerUpdate update(
        const EnemyEffectControllerContext& context);
    void bindSpawnedActor(
        std::int32_t actor_identifier,
        const EnemyEffectControllerSource& actor);

    bool active() const;
    std::int32_t counter() const;
    std::int32_t effectNumber() const;

private:
    CombatEffectSpawnRequest request_;
    std::int32_t counter_ = 0;
    std::int32_t wave_count_ = 0;
    std::int32_t wave_index_ = 0;
    std::int32_t radial_actor_count_ = 0;
    std::int32_t radial_spread_divisor_ = 0;
    bool wave_placement_blocked_ = false;
    std::array<bool, 8> radial_placement_blocked_{};
    WorldPosition type_five_position_;
    std::int32_t tracked_actor_identifier_ = -1;
    WorldPosition tracked_actor_position_;
    bool active_ = false;
};

}  // namespace osf

#endif
