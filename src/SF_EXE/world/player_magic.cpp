#include "player_magic.hpp"

#include <algorithm>

namespace osf {

void PlayerMagic::initializeNew() {
    state_.availability.fill(0);
    state_.levels.fill(1);
    state_.experience.fill(0);
    state_.bar_slots.fill(-1);
}

void PlayerMagic::restore(
    const PlayerMagicState& state) {
    state_ = state;
}

void PlayerMagic::clear() {
    state_ = {};
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

bool PlayerMagic::validSpell(std::int32_t spell) {
    return spell >= 0 &&
           static_cast<std::size_t>(spell) < spell_count;
}

bool PlayerMagic::validBarSlot(std::int32_t slot) {
    return slot >= 0 &&
           static_cast<std::size_t>(slot) < bar_slot_count;
}

}  // namespace osf
