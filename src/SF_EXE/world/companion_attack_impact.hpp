#ifndef OPENSHADOWFLARE_COMPANION_ATTACK_IMPACT_HPP
#define OPENSHADOWFLARE_COMPANION_ATTACK_IMPACT_HPP

#include "combat_packet.hpp"

#include <cstdint>

namespace osf {

class RetailRandom;

struct CompanionAttackImpactInput {
    std::int32_t source_character_number = -1;
    std::int32_t source_level = 0;
    std::int32_t physical_attack = 0;
    std::int32_t hit_rate = 0;
    std::int32_t native_element = 0;
    std::int32_t target_character_number = -1;
    std::int32_t target_physical_evasion = 0;
};

struct CompanionAttackImpactResult {
    bool valid = false;
    std::int32_t target_character_number = -1;
    std::int32_t hit_chance = 0;
    std::int32_t hit_roll = -1;
    bool show_miss = false;
    bool apply_damage = false;
    CombatPacket packet;
    std::int32_t post_hit_audio_sample = -1;
};

CombatPacket buildCompanionAttackPacket(
    const CompanionAttackImpactInput& input,
    std::int32_t hit_effect);

CompanionAttackImpactResult
resolveCompanionAttackImpact(
    const CompanionAttackImpactInput& input,
    RetailRandom& random);

}  // namespace osf

#endif
