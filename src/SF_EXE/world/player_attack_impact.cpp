#include "player_attack_impact.hpp"

#include "combat_hit_chance.hpp"
#include "core/retail_integer.hpp"
#include "core/retail_random.hpp"
#include "items/item_database.hpp"
#include "items/item_instance_values.hpp"
#include "items/player_equipment.hpp"
#include "items/player_inventory.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "player_combat_defense.hpp"
#include "player_data.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace osf {
namespace {

constexpr std::int32_t kHitAudioSample = 6;
constexpr std::int32_t kHitEffectBase = 21000;
constexpr std::int32_t kSpecialHitEffectBase = 21004;

const ItemDefinition* mainHandDefinition(
    const PlayerEquipment& equipment,
    const ItemDatabase& items) {
    const InventoryItem* main_hand =
        equipment.item(EquipmentSlot::main_hand);
    return main_hand
        ? items.find(
              main_hand->category,
              main_hand->definition_id)
        : nullptr;
}

void initializePacketDefaults(CombatPacket& packet) {
    packet.write(0, 0);
    packet.write(1, 0);
    packet.write(3, 0);
    packet.write(35, 8);
    packet.write(37, 0);
    packet.write(38, 1);
    packet.write(41, -1);
    packet.write(43, -1);
    packet.write(72, 1);
    packet.write(73, -1);
    packet.write(74, -1);
    packet.write(75, 8);
    packet.write(76, 0);
}

}  // namespace

PlayerAttackImpactStats buildPlayerAttackImpactStats(
    std::int32_t source_character_number,
    const PlayerData& player,
    const PlayerEquipment& equipment,
    const PlayerInventory& inventory,
    const ItemDatabase& items) {
    PlayerAttackImpactStats stats;
    stats.source_character_number =
        source_character_number;
    stats.level = player.level();
    stats.physical_attack = retailAdd(
        player.basePhysicalAttack(),
        equipment.derivedParameterBonus(0, items));
    stats.physical_defense = retailAdd(
        player.basePhysicalDefense(),
        equipment.derivedParameterBonus(2, items));
    stats.hit_rate = retailAdd(
        player.baseHitRate(),
        equipment.derivedParameterBonus(1, items));
    stats.state_words =
        player.combatPacketStateWords();

    PlayerCombatDefenseSnapshot defense;
    defense.character_number =
        source_character_number;
    defense.attack = stats.physical_attack;
    defense.physical_defense =
        stats.physical_defense;
    defense.element_x = player.elementX();
    defense.element_y = player.elementY();
    stats.element_affinities =
        buildPlayerElementAffinities(
            defense, equipment, inventory, items);

    const std::array<std::int32_t, 2> reflection =
        equipment.conditionalInstanceParameterBonus(
            20, 21, items);
    stats.reflection_chance = reflection[0];
    stats.reflection_percent = reflection[1];

    const InventoryItem* main_hand =
        equipment.item(EquipmentSlot::main_hand);
    const ItemDefinition* definition =
        mainHandDefinition(equipment, items);
    if (main_hand && definition) {
        stats.weapon_identifier =
            main_hand->definition_id;
        stats.weapon_subtype = definition->subtype;
        stats.reaction_chance_modifier =
            retailItemInstanceParameter(
                *main_hand, 14);
        stats.reaction_duration_modifier =
            retailItemInstanceParameter(
                *main_hand, 15);
        stats.suppress_reaction_displacement =
            retailItemInstanceParameter(
                    *main_hand, 16) != 0
            ? 1
            : 0;
    }
    return stats;
}

CombatPacket buildPlayerAttackPacket(
    const PlayerAttackImpactStats& stats,
    std::int32_t hit_effect,
    RetailRandom& random) {
    CombatPacket packet;
    initializePacketDefaults(packet);
    packet.write(2, stats.source_character_number);
    packet.write(4, stats.physical_attack);
    packet.write(5, stats.physical_defense);
    for (std::size_t index = 0;
         index < stats.element_affinities.size();
         ++index) {
        packet.write(
            index + 6,
            stats.element_affinities[index]);
    }
    for (std::size_t index = 0;
         index < stats.state_words.size();
         ++index) {
        packet.write(index + 14, stats.state_words[index]);
    }
    packet.write(31, stats.level);
    packet.write(34, hit_effect);
    packet.write(36, stats.hit_rate);
    const std::int32_t reflection_roll =
        random.next() % 100;
    packet.write(
        39,
        reflection_roll < stats.reflection_chance
            ? stats.reflection_percent
            : 0);
    packet.write(
        40, stats.suppress_reaction_displacement);
    packet.write(42, stats.reaction_chance_modifier);
    packet.write(44, stats.reaction_duration_modifier);
    packet.write(74, stats.weapon_identifier);
    return packet;
}

PlayerAttackImpactResult resolvePlayerAttackImpact(
    const PlayerAttackImpactInput& input,
    RetailRandom& random) {
    PlayerAttackImpactResult result;
    if (input.target_id < 0 ||
        input.stats.source_character_number < 0) {
        return result;
    }
    result.valid = true;
    result.target_id = input.target_id;
    result.hit_chance = retailCombatHitChance(
        input.stats.hit_rate,
        input.target_evasion);
    result.hit_roll = random.next() % 100;
    if (result.hit_roll >= result.hit_chance) {
        result.show_miss = true;
        return result;
    }

    const std::int32_t first_effect =
        random.next() % 4 + kHitEffectBase;
    result.packet = buildPlayerAttackPacket(
        input.stats, first_effect, random);
    if (input.stats.weapon_subtype == 8 ||
        input.stats.weapon_subtype == 9) {
        result.packet.write(
            34,
            random.next() % 3 +
                kSpecialHitEffectBase);
    }
    result.packet.write(
        74, input.stats.weapon_identifier);

    result.apply_damage = true;
    result.post_hit_audio_sample =
        kHitAudioSample;
    return result;
}

PlayerAttackDurabilityResult
resolvePlayerAttackDurability(
    const ItemDefinition* weapon,
    RetailRandom& random) {
    PlayerAttackDurabilityResult result;
    if (!weapon) {
        return result;
    }
    result.checked = true;
    result.roll = random.next() % 100;
    result.lose_durability =
        result.roll < 30 &&
        weapon->maximum_durability != 0;
    return result;
}

PlayerAttackApplicationResult
resolvePlayerAttackAgainstEnemy(
    const PlayerAttackImpactInput& input,
    const EnemyDamageReceiverState& enemy,
    WorldPosition impact_origin,
    const EnemyDamageReceiverContext& context,
    const ItemDefinition* weapon,
    const TableDatabase& tables,
    RetailRandom& random) {
    PlayerAttackApplicationResult result;
    result.impact =
        resolvePlayerAttackImpact(input, random);
    if (!result.impact.valid ||
        !result.impact.apply_damage) {
        return result;
    }
    result.receiver = resolveEnemyDamage(
        enemy,
        result.impact.packet,
        impact_origin,
        context,
        tables,
        random);
    result.durability =
        resolvePlayerAttackDurability(
            weapon, random);
    return result;
}

}  // namespace osf
