#ifndef OPENSHADOWFLARE_PLAYER_ATTACK_IMPACT_HPP
#define OPENSHADOWFLARE_PLAYER_ATTACK_IMPACT_HPP

#include "combat_packet.hpp"
#include "enemy_damage_receiver.hpp"

#include <array>
#include <cstdint>

namespace osf {

class ItemDatabase;
class PlayerData;
class PlayerEquipment;
class PlayerInventory;
class RetailRandom;
class TableDatabase;
struct ItemDefinition;

struct PlayerAttackImpactStats {
    std::int32_t source_character_number = -1;
    std::int32_t level = 1;
    std::int32_t physical_attack = 0;
    std::int32_t physical_defense = 0;
    std::int32_t hit_rate = 0;
    std::array<std::int32_t, 8> element_affinities{};
    std::array<std::int32_t, 17> state_words{};
    std::int32_t reflection_chance = 0;
    std::int32_t reflection_percent = 0;
    std::int32_t reaction_motion = 0;
    std::int32_t reaction_chance_modifier = 0;
    std::int32_t reaction_duration_modifier = 0;
    std::int32_t weapon_identifier = -1;
    std::int32_t weapon_subtype = -1;
};

PlayerAttackImpactStats buildPlayerAttackImpactStats(
    std::int32_t source_character_number,
    const PlayerData& player,
    const PlayerEquipment& equipment,
    const PlayerInventory& inventory,
    const ItemDatabase& items);

struct PlayerAttackImpactInput {
    PlayerAttackImpactStats stats;
    std::int32_t target_id = -1;
    std::int32_t target_evasion = 0;
};

struct PlayerAttackImpactResult {
    bool valid = false;
    std::int32_t target_id = -1;
    std::int32_t hit_chance = 0;
    std::int32_t hit_roll = -1;
    bool show_miss = false;
    bool apply_damage = false;
    CombatPacket packet;
    std::int32_t post_hit_audio_sample = -1;
};

PlayerAttackImpactResult resolvePlayerAttackImpact(
    const PlayerAttackImpactInput& input,
    RetailRandom& random);

struct PlayerAttackDurabilityResult {
    bool checked = false;
    std::int32_t roll = -1;
    bool lose_durability = false;
};

PlayerAttackDurabilityResult
resolvePlayerAttackDurability(
    const ItemDefinition* weapon,
    RetailRandom& random);

struct PlayerAttackApplicationResult {
    PlayerAttackImpactResult impact;
    EnemyDamageReceiverResult receiver;
    PlayerAttackDurabilityResult durability;
};

PlayerAttackApplicationResult
resolvePlayerAttackAgainstEnemy(
    const PlayerAttackImpactInput& input,
    const EnemyDamageReceiverState& enemy,
    WorldPosition impact_origin,
    const EnemyDamageReceiverContext& context,
    const ItemDefinition* weapon,
    const TableDatabase& tables,
    RetailRandom& random);

}  // namespace osf

#endif
