#ifndef OPENSHADOWFLARE_ENEMY_EFFECT_CONTROLLER_HPP
#define OPENSHADOWFLARE_ENEMY_EFFECT_CONTROLLER_HPP

#include "combat_effect_request.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "runtime_effect_actor.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace osf {

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
