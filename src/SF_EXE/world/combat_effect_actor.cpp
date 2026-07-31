#include "combat_effect_actor.hpp"

#include "resources/effect_visual_resource.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

namespace osf {
namespace {

constexpr std::int32_t kFirstSimpleEffect = 21000;
constexpr std::int32_t kLastSimpleEffect = 21020;
constexpr std::int32_t kFixedEffectDuration = 120;
constexpr std::int32_t kFixedEffectFadeStart = 90;
constexpr std::int32_t kFixedEffectStrength = 500;

constexpr std::array<std::int32_t, 21>
    kSimpleEffectResources{{
        11000000,
        11000001,
        11000002,
        11000009,
        11000017,
        11000018,
        11000019,
        11000020,
        11000021,
        11000022,
        11000023,
        11000024,
        11000025,
        11000026,
        11000027,
        -1,
        -1,
        -1,
        -1,
        11000050,
        11000060,
    }};

bool fixedDurationEffect(std::int32_t effect_number) {
    return effect_number >= 21010 &&
           effect_number <= 21012;
}

const gapi::CafDirection* selectedDirection(
    const EffectVisualResource& visual,
    std::int32_t direction) {
    if (visual.animation().charts().empty() ||
        direction < 0 || direction >= 9) {
        return nullptr;
    }
    return &visual.animation()
                .charts()
                .front()
                .directions[
                    static_cast<std::size_t>(direction)];
}

}  // namespace

std::int32_t retailCombatEffectResourceId(
    std::int32_t effect_number) {
    switch (effect_number) {
    case 21025:
        return 11000100;
    case 21028:
        return 11000230;
    default:
        break;
    }
    if (effect_number < kFirstSimpleEffect ||
        effect_number > kLastSimpleEffect) {
        return -1;
    }
    return kSimpleEffectResources[
        static_cast<std::size_t>(
            effect_number - kFirstSimpleEffect)];
}

bool CombatEffectActor::initialize(
    const CombatEffectSpawnRequest& request,
    WorldPosition position,
    ObjectBounds judgement,
    const EffectVisualResource& visual) {
    *this = {};
    const std::int32_t resource_id =
        retailCombatEffectResourceId(
            request.effect_number);
    const gapi::CafDirection* direction =
        selectedDirection(visual, request.packet_kind);
    if (!request.valid || resource_id < 0 ||
        !direction || direction->frame_count < 1) {
        return false;
    }

    effect_number_ = request.effect_number;
    resource_id_ = resource_id;
    position_ = position;
    judgement_ = judgement;
    direction_ = request.packet_kind;
    display_height_ = request.constructor_value_7;
    fixed_duration_ =
        fixedDurationEffect(effect_number_);
    duration_ =
        fixed_duration_
            ? kFixedEffectDuration
            : direction->frame_count;
    draw_strength_ =
        fixed_duration_
            ? kFixedEffectStrength
            : 1000;
    visual_ = &visual;
    return true;
}

void CombatEffectActor::update() {
    if (expired_ || !visual_) {
        return;
    }
    ++counter_;
    if (counter_ >= duration_) {
        expired_ = true;
        return;
    }

    const gapi::CafDirection* direction =
        selectedDirection(*visual_, direction_);
    if (!direction || direction->frame_count < 1) {
        expired_ = true;
        return;
    }
    animation_frame_ = counter_;
    animation_frame_ = std::clamp(
        animation_frame_,
        0,
        static_cast<std::int32_t>(
            direction->frame_count - 1));

    if (fixed_duration_ &&
        counter_ > kFixedEffectFadeStart) {
        draw_strength_ = std::max<std::int32_t>(
            (duration_ - counter_) *
                kFixedEffectStrength /
                (duration_ - kFixedEffectFadeStart),
            0);
    }
}

std::int32_t CombatEffectActor::effectNumber() const {
    return effect_number_;
}

std::int32_t CombatEffectActor::resourceId() const {
    return resource_id_;
}

WorldPosition CombatEffectActor::position() const {
    return position_;
}

const ObjectBounds& CombatEffectActor::judgement() const {
    return judgement_;
}

std::int32_t CombatEffectActor::animationChart() const {
    return 0;
}

std::int32_t CombatEffectActor::direction() const {
    return direction_;
}

std::int32_t CombatEffectActor::animationFrame() const {
    return animation_frame_;
}

std::int32_t CombatEffectActor::displayHeight() const {
    return display_height_;
}

std::int32_t CombatEffectActor::drawStrength() const {
    return draw_strength_;
}

bool CombatEffectActor::expired() const {
    return expired_;
}

bool CombatEffectActor::partEnabled(std::size_t part) const {
    return visual_ &&
           part < visual_->animation().maxPartCount();
}

const gapi::NjpImage& CombatEffectActor::patterns() const {
    return visual_->patterns();
}

const gapi::CafAnimation&
CombatEffectActor::animation() const {
    return visual_->animation();
}

}  // namespace osf
