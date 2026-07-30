#include "player_attack_action.hpp"

#include "items/item_database.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>

namespace osf {
namespace {

constexpr std::int16_t kImpactStatus = 0x40;

constexpr std::array<double, 10> kAttackSpeedFactors{{
    0.6,
    0.7,
    0.8,
    0.9,
    1.0,
    1.1,
    1.2,
    1.3,
    1.4,
    1.5,
}};

struct AttackCharts {
    std::int32_t first = -1;
    std::int32_t recovery = -1;
};

AttackCharts chartsForAction(PlayerAttackAction action) {
    switch (action) {
    case PlayerAttackAction::basic:
    case PlayerAttackAction::weapon_8:
        return {5, 6};
    case PlayerAttackAction::weapon_9:
        return {15, 16};
    case PlayerAttackAction::weapon_10:
        return {19, 20};
    case PlayerAttackAction::ranged_19:
    case PlayerAttackAction::ranged_20:
        return {};
    }
    return {};
}

bool usesBasicCounterOrder(PlayerAttackAction action) {
    return action == PlayerAttackAction::basic;
}

std::int32_t swingSoundCounter(PlayerAttackAction action) {
    return usesBasicCounterOrder(action) ? 5 : 6;
}

}  // namespace

PlayerAttackAction retailPlayerAttackAction(
    const ItemDefinition* main_hand) {
    if (!main_hand) {
        return PlayerAttackAction::basic;
    }
    switch (main_hand->subtype) {
    case 0:
        return PlayerAttackAction::weapon_8;
    case 1:
        return PlayerAttackAction::weapon_10;
    case 3:
        return PlayerAttackAction::weapon_9;
    case 4:
        return PlayerAttackAction::ranged_19;
    case 5:
        return PlayerAttackAction::ranged_20;
    default:
        return PlayerAttackAction::basic;
    }
}

bool playerAttackActionIsSupported(PlayerAttackAction action) {
    const AttackCharts charts = chartsForAction(action);
    return charts.first >= 0 && charts.recovery >= 0;
}

bool buildPlayerAttackAnimationTiming(
    const gapi::CafAnimation& animation,
    PlayerAttackAction action,
    std::int32_t direction,
    PlayerAttackAnimationTiming& timing) {
    timing = {};
    const AttackCharts charts = chartsForAction(action);
    if (charts.first < 0 ||
        charts.recovery < 0 ||
        direction < 0 ||
        direction >= 9 ||
        static_cast<std::size_t>(charts.first) >=
            animation.charts().size() ||
        static_cast<std::size_t>(charts.recovery) >=
            animation.charts().size()) {
        return false;
    }

    const gapi::CafDirection& first_direction =
        animation.charts()[
            static_cast<std::size_t>(charts.first)]
            .directions[static_cast<std::size_t>(direction)];
    const gapi::CafDirection& recovery_direction =
        animation.charts()[
            static_cast<std::size_t>(charts.recovery)]
            .directions[static_cast<std::size_t>(direction)];
    if (first_direction.frame_count <= 0 ||
        recovery_direction.frame_count <= 0) {
        return false;
    }

    timing.first_chart = charts.first;
    timing.recovery_chart = charts.recovery;
    timing.first_frame_count = first_direction.frame_count;
    timing.recovery_frame_count =
        recovery_direction.frame_count;
    if (!first_direction.parts.empty()) {
        const std::vector<gapi::CafCell>& cells =
            first_direction.parts.front();
        timing.first_frame_statuses.reserve(cells.size());
        for (const gapi::CafCell& cell : cells) {
            timing.first_frame_statuses.push_back(cell.status);
        }
    }
    return true;
}

std::int32_t retailPlayerAttackSpeedTier(
    std::int32_t derived_attack_speed,
    std::int32_t equipped_weight,
    std::int32_t weight_capacity,
    const TableData* speed_table) {
    if (equipped_weight > weight_capacity) {
        return 0;
    }
    const std::int32_t clamped_speed =
        std::clamp(derived_attack_speed, 0, 255);
    const std::int32_t row = clamped_speed / 32;
    const std::int32_t table_value =
        speed_table ? speed_table->value(row, 0) : -1;
    return std::clamp(table_value + 1, 0, 9);
}

bool PlayerAttackActionController::start(
    PlayerAttackAction action,
    std::int32_t target_id,
    std::int32_t attack_speed_tier,
    PlayerAttackAnimationTiming timing,
    PlayerAttackActionEvent* event) {
    cancel();
    if (target_id < 0 ||
        !playerAttackActionIsSupported(action) ||
        timing.first_frame_count <= 0 ||
        timing.recovery_frame_count <= 0) {
        return false;
    }

    action_ = action;
    target_id_ = target_id;
    attack_speed_tier_ =
        std::clamp(attack_speed_tier, 0, 9);
    timing_ = std::move(timing);
    action_counter_ = 0;
    displayed_frame_ = 0;
    previous_scanned_frame_ = -1;
    active_ = true;
    selectRenderedFrame();
    const PlayerAttackActionEvent initial_event =
        eventForCurrentFrame();
    if (event) {
        *event = initial_event;
    }
    return true;
}

PlayerAttackActionEvent PlayerAttackActionController::update(
    std::int32_t attack_speed_tier) {
    if (!active_) {
        return {};
    }
    if (attack_speed_tier >= 0) {
        attack_speed_tier_ =
            std::clamp(attack_speed_tier, 0, 9);
    }

    const double factor =
        kAttackSpeedFactors[
            static_cast<std::size_t>(attack_speed_tier_)];
    if (usesBasicCounterOrder(action_)) {
        displayed_frame_ = static_cast<std::int32_t>(
            static_cast<double>(action_counter_) * factor);
        ++action_counter_;
    } else {
        ++action_counter_;
        displayed_frame_ = static_cast<std::int32_t>(
            static_cast<double>(action_counter_) * factor);
    }

    selectRenderedFrame();
    PlayerAttackActionEvent event = eventForCurrentFrame();
    event.swing_sound_due =
        action_counter_ == swingSoundCounter(action_);
    if (animation_chart_ == timing_.recovery_chart &&
        animation_frame_ ==
            timing_.recovery_frame_count - 1) {
        event.completed = true;
        active_ = false;
    }
    return event;
}

void PlayerAttackActionController::cancel() {
    timing_ = {};
    action_ = PlayerAttackAction::basic;
    target_id_ = -1;
    attack_speed_tier_ = 0;
    action_counter_ = 0;
    displayed_frame_ = 0;
    previous_scanned_frame_ = -1;
    animation_chart_ = 0;
    animation_frame_ = 0;
    active_ = false;
}

bool PlayerAttackActionController::active() const {
    return active_;
}

PlayerAttackAction PlayerAttackActionController::action() const {
    return action_;
}

std::int32_t PlayerAttackActionController::targetId() const {
    return target_id_;
}

std::int32_t
PlayerAttackActionController::animationChart() const {
    return animation_chart_;
}

std::int32_t
PlayerAttackActionController::animationFrame() const {
    return animation_frame_;
}

std::int32_t
PlayerAttackActionController::actionCounter() const {
    return action_counter_;
}

std::int32_t
PlayerAttackActionController::displayedFrame() const {
    return displayed_frame_;
}

PlayerAttackActionEvent
PlayerAttackActionController::eventForCurrentFrame() {
    PlayerAttackActionEvent event;
    event.action = action_;
    event.target_id = target_id_;
    if (displayed_frame_ < timing_.first_frame_count &&
        displayed_frame_ != previous_scanned_frame_) {
        for (std::int32_t frame =
                 previous_scanned_frame_ + 1;
             frame <= displayed_frame_;
             ++frame) {
            if (frame >= 0 &&
                static_cast<std::size_t>(frame) <
                    timing_.first_frame_statuses.size() &&
                (timing_.first_frame_statuses[
                     static_cast<std::size_t>(frame)] &
                 kImpactStatus) != 0) {
                event.impact_due = true;
            }
        }
    }
    previous_scanned_frame_ = displayed_frame_;
    return event;
}

void PlayerAttackActionController::selectRenderedFrame() {
    animation_chart_ = timing_.first_chart;
    animation_frame_ = displayed_frame_;
    if (animation_frame_ >= timing_.first_frame_count) {
        animation_frame_ -= timing_.first_frame_count;
        animation_chart_ = timing_.recovery_chart;
        animation_frame_ = std::min(
            animation_frame_,
            timing_.recovery_frame_count - 1);
    }
}

}  // namespace osf
