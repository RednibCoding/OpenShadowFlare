#include "player_level_up_notice.hpp"

namespace osf {

bool PlayerLevelUpNotice::active() const {
    return counter > 0 && !text.empty();
}

bool PlayerLevelUpNotice::dismissible() const {
    return active() && counter < 870;
}

void PlayerLevelUpNotice::update() {
    if (counter <= 0) {
        return;
    }
    --counter;
    if (counter == 0) {
        text.clear();
    }
}

void PlayerLevelUpNotice::dismiss() {
    counter = 0;
    text.clear();
}

}  // namespace osf
