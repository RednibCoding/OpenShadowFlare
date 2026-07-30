#ifndef OPENSHADOWFLARE_ENEMY_DIRECT_IMPACT_HPP
#define OPENSHADOWFLARE_ENEMY_DIRECT_IMPACT_HPP

#include "combat_packet.hpp"
#include "enemy_effect_impact.hpp"
#include "enemy_presentation_profile.hpp"
#include "enemy_target_selector.hpp"

#include <cstdint>

namespace osf {

class RetailRandom;

struct EnemyDirectImpactInput {
    std::int32_t source_character_number = -1;
    WorldPosition source_position;
    double direction_radians = 0.0;
    std::int32_t event_number = -1;
    std::int32_t variant = -1;
    const EnemyPresentationProfile* profile = nullptr;
    EnemyAiTarget target;
};

struct EnemyDirectImpactResult {
    bool valid = false;
    bool special_effect = false;
    EnemyAiTarget target;
    CombatPacket packet;
    std::int32_t hit_chance = 0;
    std::int32_t hit_roll = -1;
    bool apply_damage = false;
    WorldPosition damage_origin;
    bool show_miss = false;
    std::int32_t post_hit_audio_sample = -1;
    std::int32_t post_hit_event = -1;
    bool player_damage_can_abort_post_hit = false;
    EnemyEffectSpawnRequest effect_spawn;
};

bool enemyDirectImpactUsesSpecialEffect(
    const EnemyPresentationProfile& profile,
    std::int32_t variant);

std::int32_t retailEnemyHitChance(
    std::int32_t attack_value,
    std::int32_t defense_value);

EnemyDirectImpactResult resolveEnemyDirectImpact(
    const EnemyDirectImpactInput& input,
    RetailRandom& random);

}  // namespace osf

#endif
