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

bool PlayerMoonSpell::toggle(
    std::int32_t effective_level,
    const TableDatabase& tables) {
    if (active()) {
        mana_change_rate_ = 0;
        return false;
    }

    const TableData* table = tables.find(kMoonTable);
    const std::int32_t column = effective_level - 1;
    if (!table || effective_level < 1 ||
        !table->contains(13, column)) {
        return false;
    }
    effective_level_ = effective_level;
    mana_change_rate_ = table->value(0, column);
    return active();
}

PlayerMoonManaUpdate PlayerMoonSpell::updateMana(
    std::int32_t current_mana,
    std::int32_t maximum_mana) {
    PlayerMoonManaUpdate result;
    result.mana = current_mana;
    const bool update_due = update_counter_ % 3 == 0;
    update_counter_ = retailAdd(update_counter_, 1);
    if (!active()) {
        mana_remainder_ = 0;
        return result;
    }

    if (update_due) {
        std::int32_t scaled =
            retailMultiply(maximum_mana, mana_change_rate_) /
            100;
        if (scaled < 0) {
            mana_remainder_ = retailSubtract(
                mana_remainder_, (-scaled) % 100);
        } else {
            mana_remainder_ = retailAdd(
                mana_remainder_, scaled % 100);
        }
        if (mana_remainder_ <= -100) {
            scaled = retailSubtract(scaled, 100);
            mana_remainder_ = retailAdd(
                mana_remainder_, 100);
        } else if (mana_remainder_ >= 100) {
            scaled = retailAdd(scaled, 100);
            mana_remainder_ = retailSubtract(
                mana_remainder_, 100);
        }
        result.mana = std::clamp(
            retailAdd(current_mana, scaled / 100),
            0,
            std::max(maximum_mana, 0));
        result.changed = result.mana != current_mana;
    }

    if (result.mana == 0) {
        mana_change_rate_ = 0;
        result.deactivated = true;
    }
    return result;
}

void PlayerMoonSpell::updateAura(bool displayed) {
    if (active() && displayed) {
        aura_frame_ = retailAdd(aura_frame_, 1);
    }
}

void PlayerMoonSpell::clear() {
    *this = {};
}

bool PlayerMoonSpell::active() const {
    return mana_change_rate_ != 0;
}

std::int32_t PlayerMoonSpell::effectiveLevel() const {
    return effective_level_;
}

std::int32_t PlayerMoonSpell::manaChangeRate() const {
    return mana_change_rate_;
}

std::int32_t PlayerMoonSpell::auraFrame() const {
    return aura_frame_;
}

CompanionProfile applyPlayerMoonCompanionModifiers(
    const CompanionProfile& base,
    const PlayerMoonSpell& moon,
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
        0,
        255);
    const std::int32_t walking_speed_raw = std::clamp(
        adjustedParameter(
            base.walking_speed_raw, *table, 2, column),
        0,
        255);
    const std::int32_t running_speed_raw = std::clamp(
        adjustedParameter(
            base.running_speed_raw, *table, 3, column),
        0,
        255);
    result.walking_speed_raw = walking_speed_raw;
    result.running_speed_raw = running_speed_raw;
    result.walking_speed = walking_speed_raw / 5;
    result.running_speed = running_speed_raw / 5;
    result.physical_attack = std::max(
        adjustedParameter(
            base.physical_attack, *table, 4, column),
        1);
    result.maximum_life = std::max(
        adjustedParameter(
            base.maximum_life, *table, 5, column),
        1);
    result.hit_rate = std::max(
        adjustedParameter(base.hit_rate, *table, 6, column),
        1);
    result.physical_defense = std::max(
        adjustedParameter(
            base.physical_defense, *table, 7, column),
        1);
    result.physical_evasion = std::max(
        adjustedParameter(
            base.physical_evasion, *table, 8, column),
        1);
    result.magical_attack = std::max(
        adjustedParameter(
            base.magical_attack, *table, 9, column),
        1);
    result.magical_hit_rate = std::max(
        adjustedParameter(
            base.magical_hit_rate, *table, 10, column),
        1);
    result.magical_evasion = std::max(
        adjustedParameter(
            base.magical_evasion, *table, 11, column),
        1);
    result.magical_defense = std::max(
        adjustedParameter(
            base.magical_defense, *table, 12, column),
        1);
    result.parameter_17 = std::max(
        adjustedParameter(
            base.parameter_17, *table, 13, column),
        1);
    return result;
}

}  // namespace osf
