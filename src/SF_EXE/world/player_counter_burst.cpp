#include "player_counter_burst.hpp"

#include "core/retail_integer.hpp"

namespace osf {

bool PlayerCounterBurst::toggle() {
    active_ = !active_;
    aura_frame_ = 0;
    return active_;
}

bool PlayerCounterBurst::deactivate() {
    const bool changed = active_;
    active_ = false;
    return changed;
}

void PlayerCounterBurst::restoreActive(bool active) {
    active_ = active;
}

void PlayerCounterBurst::updateAura(bool displayed) {
    if (active_ && displayed) {
        aura_frame_ = retailAdd(aura_frame_, 1);
    }
}

void PlayerCounterBurst::clear() {
    *this = {};
}

bool PlayerCounterBurst::active() const {
    return active_;
}

std::int32_t PlayerCounterBurst::auraFrame() const {
    return aura_frame_;
}

}  // namespace osf
