#include "player_moon_spell.hpp"

#include "core/retail_integer.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"

#include <algorithm>
#include <cstdint>

namespace osf {
namespace {

constexpr std::int32_t kMoonTable = 200;

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

CompanionProfile applyPlayerMoonCompanionModifiers(
    const CompanionProfile& base,
    const PlayerSustainedSpell& moon,
    const TableDatabase& tables) {
    CompanionProfile result = base;
    const TableData* table = tables.find(kMoonTable);
    const std::int32_t column = moon.effectiveLevel() - 1;
    if (!moon.active() || !table || column < 0 ||
        !table->contains(13, column)) {
        return result;
    }

    result.attack_speed_rating = std::clamp(
        adjustedParameter(
            base.attack_speed_rating, *table, 1, column),
         std::int32_t{0},
         std::int32_t{255});
    const std::int32_t walking_speed_raw = std::clamp(
        adjustedParameter(
            base.walking_speed_raw, *table, 2, column),
         std::int32_t{0},
         std::int32_t{255});
    const std::int32_t running_speed_raw = std::clamp(
        adjustedParameter(
            base.running_speed_raw, *table, 3, column),
         std::int32_t{0},
         std::int32_t{255});
    result.walking_speed_raw = walking_speed_raw;
    result.running_speed_raw = running_speed_raw;
    result.walking_speed = walking_speed_raw / 5;
    result.running_speed = running_speed_raw / 5;
    result.physical_attack = std::max(
        adjustedParameter(
            base.physical_attack, *table, 4, column),
         std::int32_t{1});
    result.maximum_life = std::max(
        adjustedParameter(
            base.maximum_life, *table, 5, column),
         std::int32_t{1});
    result.hit_rate = std::max(
        adjustedParameter(base.hit_rate, *table, 6, column),
         std::int32_t{1});
    result.physical_defense = std::max(
        adjustedParameter(
            base.physical_defense, *table, 7, column),
         std::int32_t{1});
    result.physical_evasion = std::max(
        adjustedParameter(
            base.physical_evasion, *table, 8, column),
         std::int32_t{1});
    result.magical_attack = std::max(
        adjustedParameter(
            base.magical_attack, *table, 9, column),
         std::int32_t{1});
    result.magical_hit_rate = std::max(
        adjustedParameter(
            base.magical_hit_rate, *table, 10, column),
         std::int32_t{1});
    result.magical_evasion = std::max(
        adjustedParameter(
            base.magical_evasion, *table, 11, column),
         std::int32_t{1});
    result.magical_defense = std::max(
        adjustedParameter(
            base.magical_defense, *table, 12, column),
         std::int32_t{1});
    result.parameter_17 = std::max(
        adjustedParameter(
            base.parameter_17, *table, 13, column),
         std::int32_t{1});
    return result;
}

}  // namespace osf
