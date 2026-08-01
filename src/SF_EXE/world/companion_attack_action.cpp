#include "companion_attack_action.hpp"

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>

namespace osf {
namespace {

constexpr std::int32_t kAttackChart = 5;
constexpr std::int16_t kImpactMarker = 0x40;
constexpr std::int16_t kSwingSoundMarker = 0x400;
constexpr std::array<double, 10> kAttackSpeedFactors{{
    0.2,
    0.3,
    0.4,
    0.5,
    0.6,
    0.7,
    0.8,
    0.9,
    1.0,
    1.1,
}};

}  // namespace

bool buildCompanionAttackAnimationTiming(
    const gapi::CafAnimation& animation,
    std::int32_t direction,
    CompanionAttackAnimationTiming& timing) {
    timing = {};
    if (direction < 0 ||
        direction >= 9 ||
        animation.charts().size() <= kAttackChart) {
        return false;
    }
    const gapi::CafDirection& chart =
        animation.charts()[kAttackChart]
            .directions[static_cast<std::size_t>(direction)];
    if (chart.frame_count <= 0) {
        return false;
    }
    timing.frame_count = chart.frame_count;
    if (!chart.parts.empty()) {
        timing.frame_statuses.reserve(
            chart.parts.front().size());
        for (const gapi::CafCell& cell :
             chart.parts.front()) {
            timing.frame_statuses.push_back(cell.status);
        }
    }
    return true;
}

std::int32_t retailCompanionAttackSpeedTier(
    std::int32_t attack_speed_rating) {
    return std::clamp(
        attack_speed_rating / 32, 0, 9);
}

bool CompanionAttackActionController::start(
    std::int32_t attack_speed_rating,
    CompanionAttackAnimationTiming timing) {
    cancel();
    if (timing.frame_count <= 0) {
        return false;
    }
    timing_ = std::move(timing);
    attack_speed_tier_ =
        retailCompanionAttackSpeedTier(
            attack_speed_rating);
    active_ = true;
    return true;
}

CompanionAttackActionEvent
CompanionAttackActionController::update() {
    CompanionAttackActionEvent event;
    if (!active_) {
        return event;
    }

    const double factor =
        kAttackSpeedFactors[
            static_cast<std::size_t>(
                attack_speed_tier_)];
    animation_frame_ =
        static_cast<std::int32_t>(
            static_cast<double>(action_counter_) *
            factor);
    ++action_counter_;

    if (animation_frame_ < timing_.frame_count &&
        animation_frame_ != previous_scanned_frame_) {
        for (std::int32_t frame =
                 previous_scanned_frame_ + 1;
             frame <= animation_frame_;
             ++frame) {
            if (frame < 0 ||
                static_cast<std::size_t>(frame) >=
                    timing_.frame_statuses.size()) {
                continue;
            }
            const std::int16_t status =
                timing_.frame_statuses[
                    static_cast<std::size_t>(frame)];
            event.impact_due =
                event.impact_due ||
                (status & kImpactMarker) != 0;
            event.swing_sound_due =
                event.swing_sound_due ||
                (status & kSwingSoundMarker) != 0;
        }
    }
    previous_scanned_frame_ = animation_frame_;
    if (animation_frame_ >= timing_.frame_count - 1) {
        animation_frame_ = timing_.frame_count - 1;
        event.completed = true;
        active_ = false;
    }
    return event;
}

void CompanionAttackActionController::cancel() {
    timing_ = {};
    attack_speed_tier_ = 0;
    action_counter_ = 0;
    animation_frame_ = 0;
    previous_scanned_frame_ = -1;
    active_ = false;
}

bool CompanionAttackActionController::active() const {
    return active_;
}

std::int32_t
CompanionAttackActionController::animationFrame() const {
    return animation_frame_;
}

std::int32_t
CompanionAttackActionController::actionCounter() const {
    return action_counter_;
}

}  // namespace osf
