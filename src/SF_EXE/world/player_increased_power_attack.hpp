#ifndef OPENSHADOWFLARE_PLAYER_INCREASED_POWER_ATTACK_HPP
#define OPENSHADOWFLARE_PLAYER_INCREASED_POWER_ATTACK_HPP

#include "combat_effect_request.hpp"
#include "player_attack_impact.hpp"

#include <cstdint>
#include <vector>

namespace osf {

class RetailRandom;

struct PlayerIncreasedPowerAttackInput {
    std::int32_t source_character_number = -1;
    PlayerAttackImpactStats stats;
    std::vector<std::int32_t> target_identifiers;
};

struct PlayerIncreasedPowerAttackResult {
    bool valid = false;
    bool consume_durability = false;
    std::vector<CombatEffectSpawnRequest> projectiles;
};

PlayerIncreasedPowerAttackResult
resolvePlayerIncreasedPowerAttack(
    const PlayerIncreasedPowerAttackInput& input,
    RetailRandom& random);

}  // namespace osf

#endif
