#include "player_magic.hpp"

#include "core/retail_integer.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"

#include <algorithm>

namespace osf {

void PlayerMagic::initializeNew() {
    state_.availability.fill(0);
    state_.levels.fill(1);
    state_.experience.fill(0);
    state_.bar_slots.fill(-1);
    temporary_bar_slots_.fill(-1);
    selected_spell_ = -1;
    targeting_ = false;
    all_spells_available_ = false;
}

void PlayerMagic::restore(
    const PlayerMagicState& state) {
    state_ = state;
    temporary_bar_slots_.fill(-1);
    selected_spell_ = -1;
    targeting_ = false;
    all_spells_available_ = false;
}

void PlayerMagic::clear() {
    state_ = {};
    state_.bar_slots.fill(-1);
    temporary_bar_slots_.fill(-1);
    selected_spell_ = -1;
    targeting_ = false;
    all_spells_available_ = false;
}

const PlayerMagicState& PlayerMagic::state() const {
    return state_;
}

bool PlayerMagic::learned(std::int32_t spell) const {
    // The Magic window and cast dispatcher both require the complete retail
    // state value, not merely either of its two low bits.
    return availability(spell) == 3;
}

std::int32_t PlayerMagic::availability(
    std::int32_t spell) const {
    return validSpell(spell)
        ? (all_spells_available_
               ? 3
               : state_.availability[
                     static_cast<std::size_t>(spell)])
        : 0;
}

std::int32_t PlayerMagic::level(
    std::int32_t spell) const {
    return validSpell(spell)
        ? state_.levels[
              static_cast<std::size_t>(spell)]
        : 0;
}

std::int32_t PlayerMagic::experience(
    std::int32_t spell) const {
    return validSpell(spell)
        ? state_.experience[
              static_cast<std::size_t>(spell)]
        : 0;
}

std::int32_t PlayerMagic::barSlot(
    std::int32_t slot) const {
    return validBarSlot(slot)
        ? (all_spells_available_
               ? temporary_bar_slots_[
                     static_cast<std::size_t>(slot)]
               : state_.bar_slots[
                     static_cast<std::size_t>(slot)])
        : -1;
}

bool PlayerMagic::assignBarSlot(
    std::int32_t slot,
    std::int32_t spell) {
    if (!validBarSlot(slot) ||
        !validSpell(spell) ||
        !learned(spell)) {
        return false;
    }
    auto& bar_slots = all_spells_available_
        ? temporary_bar_slots_
        : state_.bar_slots;
    bool changed =
        bar_slots[static_cast<std::size_t>(slot)] != spell;
    for (std::int32_t& assigned : bar_slots) {
        if (assigned == spell) {
            assigned = -1;
            changed = true;
        }
    }
    bar_slots[static_cast<std::size_t>(slot)] = spell;
    return changed;
}

bool PlayerMagic::clearBarSlot(std::int32_t slot) {
    if (!validBarSlot(slot)) {
        return false;
    }
    auto& bar_slots = all_spells_available_
        ? temporary_bar_slots_
        : state_.bar_slots;
    std::int32_t& assigned =
        bar_slots[static_cast<std::size_t>(slot)];
    if (assigned == -1) {
        return false;
    }
    assigned = -1;
    return true;
}

bool PlayerMagic::train(
    std::int32_t spell,
    bool companion_mode,
    const TableDatabase& tables) {
    if (!validSpell(spell) ||
        (state_.availability[
             static_cast<std::size_t>(spell)] & 1) == 0 ||
        level(spell) >= 20 ||
        (companion_mode
             ? spell < 7 || spell > 9
             : spell >= 7 && spell <= 9)) {
        return false;
    }
    const TableData* thresholds = tables.find(27);
    const std::int32_t column = level(spell) - 1;
    if (!thresholds ||
        !thresholds->contains(spell, column)) {
        return false;
    }

    const std::size_t index =
        static_cast<std::size_t>(spell);
    state_.experience[index] =
        retailAdd(state_.experience[index], 1);
    const std::int32_t threshold =
        thresholds->value(spell, column);
    if (state_.experience[index] < threshold) {
        return false;
    }
    state_.experience[index] =
        retailSubtract(
            state_.experience[index], threshold);
    state_.levels[index] =
        retailAdd(state_.levels[index], 1);
    return true;
}

std::int32_t PlayerMagic::selectedSpell() const {
    return selected_spell_;
}

bool PlayerMagic::selectSpell(std::int32_t spell) {
    if (spell != -1 &&
        (!validSpell(spell) || !learned(spell))) {
        return false;
    }
    if (selected_spell_ == spell) {
        return false;
    }
    selected_spell_ = spell;
    if (spell != -1) {
        targeting_ = false;
    }
    return true;
}

bool PlayerMagic::targeting() const {
    return targeting_;
}

void PlayerMagic::setTargeting(bool targeting) {
    targeting_ = targeting;
    if (targeting_) {
        selected_spell_ = -1;
    }
}

void PlayerMagic::setAllSpellsAvailable(bool available) {
    if (all_spells_available_ == available) {
        return;
    }
    all_spells_available_ = available;
    if (available) {
        temporary_bar_slots_ = state_.bar_slots;
        return;
    }
    temporary_bar_slots_.fill(-1);
    if (selected_spell_ >= 0 &&
        state_.availability[
            static_cast<std::size_t>(selected_spell_)] != 3) {
        selected_spell_ = -1;
    }
}

bool PlayerMagic::allSpellsAvailable() const {
    return all_spells_available_;
}

bool PlayerMagic::validSpell(std::int32_t spell) {
    return spell >= 0 &&
           static_cast<std::size_t>(spell) < spell_count;
}

bool PlayerMagic::validBarSlot(std::int32_t slot) {
    return slot >= 0 &&
           static_cast<std::size_t>(slot) < bar_slot_count;
}

}  // namespace osf
