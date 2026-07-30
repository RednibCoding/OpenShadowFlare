#ifndef OPENSHADOWFLARE_ENEMY_EFFECT_CONTROLLER_HPP
#define OPENSHADOWFLARE_ENEMY_EFFECT_CONTROLLER_HPP

#include "combat_effect_request.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace osf {

enum class RuntimeEffectActorAction : std::int32_t {
    source_animation = 0,
    forward = 1,
};

struct RuntimeEffectActorSpawnRequest {
    std::int32_t controller_effect_number = -1;
    std::int32_t resource_id = -1;
    std::int32_t owner_kind = 0;
    std::int32_t source_character_number = -1;
    std::int32_t target_kind = 0;
    std::int32_t target_identifier = 0;
    std::int32_t constructor_value_6 = 0;
    std::int32_t constructor_value_7 = 0;
    double direction_radians = 0.0;
    WorldPosition position;
    ObjectBounds judgement;
    RuntimeEffectActorAction action =
        RuntimeEffectActorAction::source_animation;
    std::int32_t animation_chart = 0;
    std::int32_t animation_direction = 8;
    bool has_packet = false;
    CombatPacket packet;
};

struct PositionalEffectAudioRequest {
    std::int32_t sample = -1;
    WorldPosition position;
};

struct EnemyEffectControllerSource {
    bool found = false;
    WorldPosition position;
};

struct EnemyEffectControllerUpdate {
    std::array<RuntimeEffectActorSpawnRequest, 2>
        actor_spawns;
    std::size_t actor_spawn_count = 0;
    std::array<PositionalEffectAudioRequest, 1>
        audio;
    std::size_t audio_count = 0;
    bool expired = false;
};

class EnemyEffectController {
public:
    bool initialize(
        const CombatEffectSpawnRequest& request);
    EnemyEffectControllerUpdate update(
        const EnemyEffectControllerSource& source);

    bool active() const;
    std::int32_t counter() const;
    std::int32_t effectNumber() const;

private:
    CombatEffectSpawnRequest request_;
    std::int32_t counter_ = 0;
    bool active_ = false;
};

}  // namespace osf

#endif
