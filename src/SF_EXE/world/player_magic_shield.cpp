#include "player_magic_shield.hpp"

#include "core/retail_integer.hpp"

namespace osf {

bool PlayerMagicShield::toggle() {
    active_ = !active_;
    aura_frame_ = 0;
    return active_;
}

bool PlayerMagicShield::deactivate() {
    const bool changed = active_;
    active_ = false;
    return changed;
}

void PlayerMagicShield::restoreActive(bool active) {
    active_ = active;
}

void PlayerMagicShield::updateAura(bool displayed) {
    if (active_ && displayed) {
        aura_frame_ = retailAdd(aura_frame_, 1);
    }
}

void PlayerMagicShield::clear() {
    *this = {};
}

bool PlayerMagicShield::active() const {
    return active_;
}

std::int32_t PlayerMagicShield::auraFrame() const {
    return aura_frame_;
}

}  // namespace osf
