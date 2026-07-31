#include "player_energy_shield.hpp"

#include "core/retail_integer.hpp"
#include "player_magic.hpp"

namespace osf {

bool PlayerEnergyShield::toggle(
    std::int32_t current_mana) {
    if (active_) {
        active_ = false;
        return false;
    }
    if (current_mana == 0) {
        return false;
    }
    active_ = true;
    return true;
}

bool PlayerEnergyShield::deactivate() {
    const bool changed = active_;
    active_ = false;
    return changed;
}

void PlayerEnergyShield::updateAura(bool displayed) {
    if (active_ && displayed) {
        aura_frame_ = retailAdd(aura_frame_, 1);
    }
}

void PlayerEnergyShield::clear() {
    *this = {};
}

bool PlayerEnergyShield::active() const {
    return active_;
}

std::int32_t PlayerEnergyShield::auraFrame() const {
    return aura_frame_;
}

bool trainEnergyShieldOnOwnedKill(
    PlayerMagic& magic,
    const PlayerEnergyShield& energy_shield,
    std::int32_t defeat_source_character_number,
    std::int32_t local_player_slot,
    const TableDatabase& tables) {
    if (!energy_shield.active() ||
        defeat_source_character_number < 0 ||
        defeat_source_character_number % 10 !=
            local_player_slot) {
        return false;
    }
    const std::int32_t experience_before =
        magic.experience(9);
    const std::int32_t level_before = magic.level(9);
    magic.train(9, true, tables);
    return magic.experience(9) != experience_before ||
           magic.level(9) != level_before;
}

}  // namespace osf
