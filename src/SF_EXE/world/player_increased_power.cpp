#include "player_increased_power.hpp"

#include "core/retail_integer.hpp"
#include "core/retail_random.hpp"
#include "items/player_special_items.hpp"

#include <algorithm>

namespace osf {
namespace {

constexpr std::int32_t kSpecialItemCategory = 4;
constexpr std::int32_t kEarlyActivationItem = 98000001;
constexpr std::int32_t kRangedJob = 5;
constexpr std::int32_t kOrdinaryRangedAction = 20;
constexpr std::int32_t kRedirectChance = 33;

}  // namespace

void PlayerIncreasedPower::clear() {
    charge_ = 0;
    remaining_updates_ = 0;
    aura_frame_ = 0;
}

void PlayerIncreasedPower::accountDirectLocalKill() {
    charge_ = retailAdd(charge_, 1);
}

bool PlayerIncreasedPower::ready(
    const PlayerSpecialItems& special_items) const {
    if (charge_ >= ordinary_kill_threshold) {
        return true;
    }
    return charge_ >= assisted_kill_threshold &&
        special_items.contains(
            kSpecialItemCategory, kEarlyActivationItem);
}

bool PlayerIncreasedPower::activate(
    const PlayerSpecialItems& special_items) {
    if (!ready(special_items)) {
        return false;
    }
    charge_ = 0;
    remaining_updates_ = active_updates;
    return true;
}

void PlayerIncreasedPower::deactivate() {
    remaining_updates_ = 0;
}

bool PlayerIncreasedPower::update() {
    if (!active()) {
        return false;
    }
    const bool sound_due = remaining_updates_ % 15 == 0;
    --remaining_updates_;
    return sound_due;
}

void PlayerIncreasedPower::updateAura(bool visible) {
    if (active() && visible) {
        aura_frame_ = retailAdd(aura_frame_, 1);
    }
}

bool PlayerIncreasedPower::active() const {
    return remaining_updates_ != 0;
}

std::int32_t PlayerIncreasedPower::charge() const {
    return charge_;
}

std::int32_t PlayerIncreasedPower::remainingUpdates() const {
    return remaining_updates_;
}

std::int32_t PlayerIncreasedPower::auraFrame() const {
    return aura_frame_;
}

std::int32_t PlayerIncreasedPower::movementSpeedTier(
    std::int32_t ordinary_tier) const {
    return active()
        ? 9
        : std::clamp(
              ordinary_tier,
              std::int32_t{0},
              std::int32_t{9});
}

bool PlayerIncreasedPower::blocksSpell(
    std::int32_t spell) const {
    return active() &&
        (spell == 7 || spell == 8 || spell == 9);
}

bool PlayerIncreasedPower::redirectsRangedAttack(
    std::int32_t current_job,
    std::int32_t requested_action,
    RetailRandom& random) const {
    return active() &&
        current_job == kRangedJob &&
        requested_action == kOrdinaryRangedAction &&
        random.next() % 100 < kRedirectChance;
}

}  // namespace osf
