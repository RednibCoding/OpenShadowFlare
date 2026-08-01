#include "player_runtime_profile.hpp"

#include "core/retail_integer.hpp"
#include "items/item_database.hpp"
#include "items/player_equipment.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "player_data.hpp"
#include "player_sustained_spell.hpp"

#include <algorithm>
#include <cstdint>

namespace osf {
namespace {

constexpr std::int32_t kBerserkerTable = 201;

std::int32_t adjustedParameter(
    std::int32_t value,
    const TableData& table,
    std::int32_t row,
    std::int32_t column) {
    return retailAdd(
        value,
        retailMultiply(table.value(row, column), value) /
            100);
}

}  // namespace

std::int32_t PlayerRuntimeProfile::walkingSpeedTier() const {
    return std::clamp((walking_speed_raw + 32) / 32, std::int32_t{0}, std::int32_t{9});
}

PlayerRuntimeProfile buildPlayerRuntimeProfile(
    const PlayerData& player,
    const PlayerEquipment& equipment,
    const ItemDatabase& items,
    const PlayerSustainedSpell& berserker,
    const TableDatabase& tables) {
    PlayerRuntimeProfile result;
    const PlayerEquipment::DerivedParameterBonuses
        equipment_bonuses =
        equipment.derivedParameterBonuses(items);
    result.attack_speed_raw = std::clamp(
        retailAdd(
            player.baseAttackSpeed(),
            equipment_bonuses[8]),
        std::int32_t{0},
        std::int32_t{255});
    result.walking_speed_raw = std::clamp(
        retailAdd(
            player.initialParameter(1),
            equipment_bonuses[9]),
        std::int32_t{0},
        std::int32_t{255});
    result.maximum_life =
        std::max(player.baseMaximumLife(), std::int32_t{1});
    result.maximum_mana =
        std::max(player.baseMaximumMana(), std::int32_t{1});
    result.weight_capacity =
        std::max(player.baseWeightCapacity(), std::int32_t{0});
    result.physical_attack = std::max(
        retailAdd(
            player.basePhysicalAttack(),
            equipment_bonuses[0]),
        std::int32_t{1});
    result.physical_defense = std::max(
        retailAdd(
            player.basePhysicalDefense(),
            equipment_bonuses[2]),
        std::int32_t{1});
    result.hit_rate = std::max(
        retailAdd(
            player.baseHitRate(),
            equipment_bonuses[1]),
        std::int32_t{1});
    result.physical_evasion = std::max(
        retailAdd(
            player.baseEvasionRate(),
            equipment_bonuses[3]),
        std::int32_t{1});
    result.magical_attack = std::max(
        retailAdd(
            player.baseMagicalAttack(),
            equipment_bonuses[4]),
        std::int32_t{1});
    result.magical_defense = std::max(
        retailAdd(
            player.baseMagicalDefense(),
            equipment_bonuses[6]),
        std::int32_t{1});
    result.magical_hit_rate = std::max(
        retailAdd(
            player.baseMagicalHitRate(),
            equipment_bonuses[5]),
        std::int32_t{1});
    result.magical_evasion = std::max(
        retailAdd(
            player.baseMagicalEvasionRate(),
            equipment_bonuses[7]),
        std::int32_t{1});

    const TableData* table = tables.find(kBerserkerTable);
    const std::int32_t column = berserker.effectiveLevel() - 1;
    if (!berserker.active() || !table || column < 0 ||
        !table->contains(12, column)) {
        return result;
    }

    result.attack_speed_raw = std::clamp(
        adjustedParameter(
            result.attack_speed_raw, *table, 1, column),
        std::int32_t{0},
        std::int32_t{255});
    result.walking_speed_raw = std::clamp(
        adjustedParameter(
            result.walking_speed_raw, *table, 2, column),
        std::int32_t{0},
        std::int32_t{255});
    result.maximum_life = std::max(
        adjustedParameter(
            result.maximum_life, *table, 3, column),
        std::int32_t{1});
    result.maximum_mana = std::max(
        adjustedParameter(
            result.maximum_mana, *table, 4, column),
        std::int32_t{1});
    result.physical_attack = std::max(
        adjustedParameter(
            result.physical_attack, *table, 5, column),
        std::int32_t{1});
    result.physical_defense = std::max(
        adjustedParameter(
            result.physical_defense, *table, 6, column),
        std::int32_t{1});
    result.hit_rate = std::max(
        adjustedParameter(
            result.hit_rate, *table, 7, column),
        std::int32_t{1});
    result.physical_evasion = std::max(
        adjustedParameter(
            result.physical_evasion, *table, 8, column),
        std::int32_t{1});
    result.magical_attack = std::max(
        adjustedParameter(
            result.magical_attack, *table, 9, column),
        std::int32_t{1});
    result.magical_defense = std::max(
        adjustedParameter(
            result.magical_defense, *table, 10, column),
        std::int32_t{1});
    result.magical_hit_rate = std::max(
        adjustedParameter(
            result.magical_hit_rate, *table, 11, column),
        std::int32_t{1});
    result.magical_evasion = std::max(
        adjustedParameter(
            result.magical_evasion, *table, 12, column),
        std::int32_t{1});
    return result;
}

}  // namespace osf
