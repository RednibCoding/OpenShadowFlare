#include "player_sustained_spell.hpp"

#include "core/retail_integer.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "player_magic.hpp"

namespace osf {

bool PlayerSustainedSpell::toggle(
    std::int32_t table_number,
    std::int32_t effective_level,
    const TableDatabase& tables) {
    if (active()) {
        mana_change_rate_ = 0;
        return false;
    }

    const TableData* table = tables.find(table_number);
    const std::int32_t column = effective_level - 1;
    if (!table || effective_level < 1 ||
        !table->contains(0, column)) {
        return false;
    }
    effective_level_ = effective_level;
    mana_change_rate_ = table->value(0, column);
    return active();
}

bool PlayerSustainedSpell::deactivate() {
    const bool changed = active();
    mana_change_rate_ = 0;
    return changed;
}

void PlayerSustainedSpell::updateAura(bool displayed) {
    if (active() && displayed) {
        aura_frame_ = retailAdd(aura_frame_, 1);
    }
}

void PlayerSustainedSpell::clear() {
    *this = {};
}

bool PlayerSustainedSpell::active() const {
    return mana_change_rate_ != 0;
}

std::int32_t PlayerSustainedSpell::effectiveLevel() const {
    return effective_level_;
}

std::int32_t PlayerSustainedSpell::manaChangeRate() const {
    return mana_change_rate_;
}

std::int32_t PlayerSustainedSpell::auraFrame() const {
    return aura_frame_;
}

PlayerSustainedSpellTraining
trainActiveSustainedSpellsOnOwnedKill(
    PlayerMagic& magic,
    const PlayerSustainedSpell& moon,
    const PlayerSustainedSpell& berserker,
    std::int32_t defeat_source_character_number,
    std::int32_t local_player_slot,
    const TableDatabase& tables) {
    PlayerSustainedSpellTraining result;
    if (defeat_source_character_number < 0 ||
        defeat_source_character_number % 10 !=
            local_player_slot) {
        return result;
    }
    if (moon.active()) {
        const std::int32_t before = magic.experience(7);
        const std::int32_t level_before = magic.level(7);
        magic.train(7, true, tables);
        result.moon_trained =
            magic.experience(7) != before ||
            magic.level(7) != level_before;
    }
    if (berserker.active()) {
        const std::int32_t before = magic.experience(8);
        const std::int32_t level_before = magic.level(8);
        magic.train(8, true, tables);
        result.berserker_trained =
            magic.experience(8) != before ||
            magic.level(8) != level_before;
    }
    return result;
}

PlayerSustainedSpellShutdown
deactivateSustainedSpellsAtZeroMana(
    std::int32_t current_mana,
    PlayerSustainedSpell& moon,
    PlayerSustainedSpell& berserker) {
    if (current_mana != 0) {
        return {};
    }
    return {
        moon.deactivate(),
        berserker.deactivate(),
    };
}

}  // namespace osf
