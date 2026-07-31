#ifndef OPENSHADOWFLARE_PLAYER_RANGED_ATTACK_HPP
#define OPENSHADOWFLARE_PLAYER_RANGED_ATTACK_HPP

#include "combat_effect_request.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "player_attack_impact.hpp"

#include <cstdint>
#include <vector>

namespace osf {

class RetailRandom;
struct ItemDefinition;

struct PlayerRangedAttackInput {
    std::int32_t source_character_number = -1;
    WorldPosition source_position;
    WorldPosition target_position;
    std::int32_t target_identifier = -1;
    std::int32_t current_job = -1;
    std::int32_t ranged_job_level = 0;
    PlayerAttackImpactStats stats;
    const ItemDefinition* weapon = nullptr;
};

struct PlayerRangedAttackResult {
    bool valid = false;
    bool consume_durability = false;
    std::vector<CombatEffectSpawnRequest> projectiles;
};

std::int32_t retailRangedPhysicalAttack(
    std::int32_t physical_attack,
    std::int32_t current_job,
    std::int32_t ranged_job_level);

PlayerRangedAttackResult resolvePlayerRangedAttack(
    const PlayerRangedAttackInput& input,
    RetailRandom& random);

}  // namespace osf

#endif
