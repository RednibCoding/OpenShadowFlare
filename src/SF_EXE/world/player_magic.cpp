#include "player_magic.hpp"

#include <algorithm>

namespace osf {

void PlayerMagic::initializeNew() {
    state_.availability.fill(0);
    state_.levels.fill(1);
    state_.experience.fill(0);
    state_.bar_slots.fill(-1);
    selected_spell_ = -1;
    targeting_ = false;
}

void PlayerMagic::restore(
    const PlayerMagicState& state) {
    state_ = state;
    selected_spell_ = -1;
    targeting_ = false;
}

void PlayerMagic::clear() {
    state_ = {};
    state_.bar_slots.fill(-1);
    selected_spell_ = -1;
    targeting_ = false;
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
        ? state_.availability[
              static_cast<std::size_t>(spell)]
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
        ? state_.bar_slots[
              static_cast<std::size_t>(slot)]
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
    bool changed =
        state_.bar_slots[
            static_cast<std::size_t>(slot)] != spell;
    for (std::int32_t& assigned : state_.bar_slots) {
        if (assigned == spell) {
            assigned = -1;
            changed = true;
        }
    }
    state_.bar_slots[
        static_cast<std::size_t>(slot)] = spell;
    return changed;
}

bool PlayerMagic::clearBarSlot(std::int32_t slot) {
    if (!validBarSlot(slot)) {
        return false;
    }
    std::int32_t& assigned =
        state_.bar_slots[
            static_cast<std::size_t>(slot)];
    if (assigned == -1) {
        return false;
    }
    assigned = -1;
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

bool PlayerMagic::validSpell(std::int32_t spell) {
    return spell >= 0 &&
           static_cast<std::size_t>(spell) < spell_count;
}

bool PlayerMagic::validBarSlot(std::int32_t slot) {
    return slot >= 0 &&
           static_cast<std::size_t>(slot) < bar_slot_count;
}

}  // namespace osf
