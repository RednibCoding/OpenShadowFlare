#include "enemy_presentation.hpp"

#include "actor_direction.hpp"
#include "enemy_effect_impact.hpp"
#include "enemy_presentation_audio.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace osf {
namespace {

constexpr std::int32_t kFirstPresentationAction = 1;
constexpr std::int32_t kLastPresentationAction = 6;
constexpr std::int32_t kFirstEffectAction = 4;
constexpr std::int32_t kIdlePresentationAction = 7;
constexpr std::uint16_t kImpactMarker = 0x40;

constexpr std::array<double, 10> kAnimationSpeed{{
    0.3,
    0.4,
    0.6,
    0.8,
    1.0,
    1.5,
    2.0,
    2.5,
    3.0,
    4.0,
}};

bool validAction(std::int32_t action) {
    return action >= kFirstPresentationAction &&
           action <= kLastPresentationAction;
}

bool effectAction(std::int32_t action) {
    return action >= kFirstEffectAction;
}

std::size_t variantForAction(std::int32_t action) {
    return static_cast<std::size_t>(
        effectAction(action)
            ? action - kFirstEffectAction
            : action - kFirstPresentationAction);
}

std::int32_t chartForAction(
    const EnemyPresentationProfile& profile,
    std::int32_t action) {
    const std::size_t variant =
        variantForAction(action);
    return effectAction(action)
        ? profile.effect_animation_chart[variant]
        : profile.direct_animation_chart[variant];
}

std::int32_t speedIndexForAction(
    const EnemyPresentationProfile& profile,
    std::int32_t action) {
    const std::size_t variant =
        variantForAction(action);
    return effectAction(action)
        ? profile.effect_animation_speed_index[variant]
        : profile.direct_animation_speed_index[variant];
}

std::uint8_t markerBits(std::uint16_t status) {
    std::uint8_t result = 0;
    if ((status & 0x400u) != 0) {
        result |= kEnemyAudioMarkerZero;
    }
    if ((status & 0x800u) != 0) {
        result |= kEnemyAudioMarkerOne;
    }
    if ((status & 0x1000u) != 0) {
        result |= kEnemyAudioMarkerTwo;
    }
    return result;
}

}  // namespace

void EnemyPresentationController::reset() {
    requested_action_ = kIdlePresentationAction;
    presentation_action_ = kIdlePresentationAction;
    animation_chart_ = 0;
    animation_frame_ = 0;
    direction_ = 0;
    elapsed_updates_ = 0;
    previous_animation_frame_ = -1;
    direction_radians_ = 0.0;
    target_ = {};
}

void EnemyPresentationController::select(
    std::int32_t presentation_action) {
    requested_action_ = presentation_action;
}

EnemyPresentationUpdate
EnemyPresentationController::update(
    const EnemyPresentationContext& context) {
    EnemyPresentationUpdate result;
    result.presentation_action = presentation_action_;
    result.animation_chart = animation_chart_;
    result.animation_frame = animation_frame_;
    result.direction = direction_;
    result.target = target_;

    if (!validAction(requested_action_) ||
        !context.profile) {
        return result;
    }
    const std::int32_t speed_index =
        speedIndexForAction(
            *context.profile,
            requested_action_);
    if (speed_index < 0 ||
        static_cast<std::size_t>(speed_index) >=
            kAnimationSpeed.size()) {
        return result;
    }

    const bool entering =
        presentation_action_ != requested_action_;
    if (entering) {
        presentation_action_ = requested_action_;
        animation_chart_ = chartForAction(
            *context.profile,
            presentation_action_);
        animation_frame_ = 0;
        elapsed_updates_ = 0;
        previous_animation_frame_ = -1;
        direction_ = context.direction;
        direction_radians_ =
            retailAngleForDirection(context.direction);
        target_ = {};

        const std::size_t variant =
            variantForAction(presentation_action_);
        if (effectAction(presentation_action_)) {
            if (context.default_target) {
                target_ = context.default_target();
            }
        } else if (context.target_in_range) {
            target_ = context.target_in_range(
                0,
                context.profile
                    ->direct_maximum_target_distance[
                        variant]);
        }
        if (target_.found) {
            direction_radians_ =
                retailAngleForVector(
                    target_.position.x -
                        context.position.x,
                    target_.position.y -
                        context.position.y);
            direction_ = retailDirectionForVector(
                target_.position.x - context.position.x,
                target_.position.y - context.position.y);
        }
    } else {
        ++elapsed_updates_;
        animation_frame_ = static_cast<std::int32_t>(
            static_cast<double>(elapsed_updates_) *
            kAnimationSpeed[
                static_cast<std::size_t>(speed_index)]);
    }

    result.handled = true;
    result.active = true;
    result.presentation_action = presentation_action_;
    result.animation_chart = animation_chart_;
    result.animation_frame = animation_frame_;
    result.direction = direction_;
    result.target = target_;

    std::int32_t frame_count = 0;
    const gapi::CafDirection* animation_direction = nullptr;
    if (context.animation &&
        animation_chart_ >= 0 &&
        static_cast<std::size_t>(animation_chart_) <
            context.animation->charts().size() &&
        direction_ >= 0 &&
        direction_ < 9) {
        const gapi::CafChart& chart =
            context.animation->charts()[
                static_cast<std::size_t>(
                    animation_chart_)];
        animation_direction =
            &chart.directions[
                static_cast<std::size_t>(direction_)];
        frame_count = animation_direction->frame_count;
    }

    if (animation_direction &&
        frame_count > 0 &&
        animation_frame_ < frame_count &&
        animation_frame_ !=
            previous_animation_frame_ &&
        !animation_direction->parts.empty()) {
        const std::vector<gapi::CafCell>& cells =
            animation_direction->parts.front();
        for (std::int32_t frame =
                 previous_animation_frame_ + 1;
             frame <= animation_frame_;
             ++frame) {
            if (frame < 0 ||
                static_cast<std::size_t>(frame) >=
                    cells.size()) {
                continue;
            }
            const std::uint16_t status =
                static_cast<std::uint16_t>(
                    cells[
                        static_cast<std::size_t>(
                            frame)]
                        .status);
            result.audio_markers |= markerBits(status);
            if ((status & kImpactMarker) != 0) {
                result.impact = true;
            }
        }
    }
    for (std::int32_t marker_slot = 0;
         marker_slot < 3;
         ++marker_slot) {
        if ((result.audio_markers &
             (1u << static_cast<std::uint32_t>(
                 marker_slot))) == 0) {
            continue;
        }
        result.audio_samples[
            static_cast<std::size_t>(marker_slot)] =
            retailEnemyPresentationSample(
                context.resource_id,
                animation_chart_,
                marker_slot);
    }

    if (frame_count > 0 &&
        animation_frame_ >= frame_count) {
        animation_frame_ = frame_count - 1;
        result.animation_frame = animation_frame_;
    }

    if (result.impact) {
        const std::size_t variant =
            variantForAction(presentation_action_);
        result.impact_family =
            effectAction(presentation_action_)
                ? EnemyPresentationFamily::effect
                : EnemyPresentationFamily::direct;
        result.impact_variant = static_cast<std::int32_t>(
            variant);
        if (result.impact_family ==
            EnemyPresentationFamily::direct) {
            const bool special_effect =
                enemyDirectImpactUsesSpecialEffect(
                    *context.profile,
                    static_cast<std::int32_t>(
                        variant));
            if (!special_effect &&
                context.direct_impact_target) {
                result.direct_impact_target =
                    context.direct_impact_target(
                        context.profile
                            ->direct_maximum_target_distance[
                                variant],
                        direction_);
            }
            if (context.random) {
                result.direct_impact =
                    resolveEnemyDirectImpact(
                        {
                            context
                                .source_character_number,
                            context.position,
                            direction_radians_,
                            context.event_number,
                            static_cast<std::int32_t>(
                                variant),
                            context.profile,
                            result.direct_impact_target,
                        },
                        *context.random);
            }
        } else {
            result.effect_type =
                context.profile->effect_type[variant];
            result.effect_subtype =
                context.profile->effect_subtype[variant];
            result.effect_parameter =
                context.profile->effect_parameter[variant];
            result.effect_additive =
                context.profile->effect_additive[variant];
            if (result.effect_type == 12 &&
                context.default_target) {
                result.effect_impact_target =
                    context.default_target();
            }
            if (context.parameter_tables &&
                context.random) {
                result.effect_spawn =
                    resolveEnemyEffectImpact(
                        {
                            context
                                .source_character_number,
                            context.position,
                            context.source_judgement,
                            direction_radians_,
                            result.effect_type,
                            result.effect_subtype,
                            result.effect_parameter,
                            result.effect_additive,
                            context.profile
                                ->packet_word_31,
                            result.effect_impact_target,
                        },
                        *context.parameter_tables,
                        *context.random);
            }
        }
    }
    if (!context.animation ||
        frame_count <= 0 ||
        animation_frame_ == frame_count - 1) {
        result.active = false;
        const bool impact_event_precedes_completion =
            result.direct_impact.post_hit_event != -1;
        if (context.event_number == -1 &&
            !impact_event_precedes_completion) {
            result.completion_event =
                presentation_action_ + 1;
        }
        requested_action_ = kIdlePresentationAction;
        presentation_action_ = kIdlePresentationAction;
        animation_chart_ = 0;
    }

    previous_animation_frame_ = animation_frame_;
    return result;
}

std::int32_t
EnemyPresentationController::presentationAction() const {
    return presentation_action_;
}

std::int32_t
EnemyPresentationController::animationChart() const {
    return animation_chart_;
}

std::int32_t
EnemyPresentationController::animationFrame() const {
    return animation_frame_;
}

std::int32_t EnemyPresentationController::direction() const {
    return direction_;
}

std::int32_t
EnemyPresentationController::elapsedUpdates() const {
    return elapsed_updates_;
}

std::int32_t
EnemyPresentationController::previousAnimationFrame() const {
    return previous_animation_frame_;
}

const EnemyAiTarget&
EnemyPresentationController::target() const {
    return target_;
}

}  // namespace osf
