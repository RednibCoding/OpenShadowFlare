#include "companion_actor.hpp"

#include "actor_direction.hpp"
#include "resources/character_visual_resource.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace osf {
namespace {

constexpr std::int32_t kFirstCompanionCharacterNumber =
    16000000;
constexpr std::int32_t kCompanionCloseDistance = 160;
constexpr std::int32_t kCompanionRunDistance = 600;
constexpr std::int32_t kCompanionTeleportDistance = 4000;
constexpr std::int32_t kCompanionCloseLinger = 5;
constexpr std::int32_t kCompanionTeleportOffset = 200;
constexpr std::int32_t kHitChart = 3;
constexpr std::int32_t kDeathChart = 4;
constexpr std::int32_t kReviveChart = 7;
constexpr std::int32_t kSpecialDirection = 8;
constexpr std::int32_t kHitDisplacement = 120;
constexpr std::int32_t kDeathFadeUpdates = 60;

std::int32_t chartForMotion(CompanionMotion motion) {
    switch (motion) {
    case CompanionMotion::walking:
        return 1;
    case CompanionMotion::running:
        return 2;
    case CompanionMotion::attacking:
        return 5;
    case CompanionMotion::exploding:
        return 6;
    case CompanionMotion::reacting:
        return kHitChart;
    case CompanionMotion::defeated:
        return kDeathChart;
    case CompanionMotion::reviving:
        return kReviveChart;
    case CompanionMotion::idle:
    default:
        return 0;
    }
}

std::int32_t frameCount(
    const gapi::CafAnimation& animation,
    std::int32_t chart,
    std::int32_t direction) {
    if (chart < 0 || direction < 0 ||
        direction >= 9 ||
        static_cast<std::size_t>(chart) >=
            animation.charts().size()) {
        return 0;
    }
    return animation.charts()[
               static_cast<std::size_t>(chart)]
        .directions[
            static_cast<std::size_t>(direction)]
        .frame_count;
}

}  // namespace

bool CompanionActor::initialize(
    const CompanionProfile& profile,
    const CharacterVisualResource& visual,
    std::int32_t owner_slot,
    WorldPosition position,
    std::int32_t direction) {
    clear();
    if (profile.type < 0 ||
        profile.resource_id < 0 ||
        profile.maximum_life < 1 ||
        owner_slot < 0 ||
        owner_slot > 3) {
        return false;
    }
    profile_ = profile;
    owner_slot_ = owner_slot;
    position_ = position;
    previous_position_ = position;
    direction_ = direction;
    current_life_ = profile.maximum_life;
    draw_opacity_ = 1000;
    visual_ = &visual;
    return true;
}

void CompanionActor::clear() {
    profile_ = {};
    owner_slot_ = -1;
    position_ = {};
    previous_position_ = {};
    direction_ = 0;
    motion_ = CompanionMotion::idle;
    action_counter_ = 0;
    close_linger_counter_ = 0;
    current_life_ = 0;
    presentation_action_ = 2;
    presentation_counter_ = 0;
    presentation_animation_frame_ = 0;
    action_lock_ = 0;
    reaction_duration_ = 0;
    reaction_stage_ = 0;
    reaction_displacement_suppressed_ = false;
    reaction_additive_ = 0;
    reaction_angle_ = 0.0;
    event_number_ = 0;
    draw_opacity_ = 1000;
    combat_target_character_number_ = -1;
    movement_controller_.reset();
    attack_action_.cancel();
    explosion_action_.cancel();
    pending_explosion_destination_ = {};
    explosion_pending_ = false;
    visual_ = nullptr;
}

void CompanionActor::relocate(
    WorldPosition position,
    std::int32_t direction) {
    position_ = position;
    previous_position_ = position;
    direction_ = direction;
    close_linger_counter_ = 0;
    combat_target_character_number_ = -1;
    movement_controller_.reset();
    attack_action_.cancel();
    explosion_action_.cancel();
    pending_explosion_destination_ = {};
    explosion_pending_ = false;
    if (current_life_ > 0) {
        presentation_action_ = 2;
        presentation_counter_ = 0;
        presentation_animation_frame_ = 0;
        action_lock_ = 0;
    }
    selectMotion(CompanionMotion::idle);
    if (current_life_ <= 0) {
        beginDefeatedWait();
    }
}

void CompanionActor::updateFollow(
    WorldPosition owner_position,
    const ObjectBounds& owner_bounds,
    const GroundMap& ground,
    const ObjectMap& objects,
    const std::vector<MovementBlocker>*
        dynamic_blockers) {
    if (!valid()) {
        return;
    }
    previous_position_ = position_;
    const std::int32_t distance =
        distanceBetweenBounds(
            position_,
            judgement_,
            owner_position,
            owner_bounds);
    if (distance >= kCompanionTeleportDistance) {
        position_ = {
            owner_position.x + kCompanionTeleportOffset,
            owner_position.y + kCompanionTeleportOffset,
        };
        previous_position_ = position_;
        movement_controller_.reset();
        ++action_counter_;
        return;
    }

    if (distance < kCompanionCloseDistance) {
        selectMotion(CompanionMotion::idle);
        close_linger_counter_ = kCompanionCloseLinger;
        ++action_counter_;
        return;
    }
    if (distance < kCompanionRunDistance) {
        if (close_linger_counter_ > 0) {
            --close_linger_counter_;
            ++action_counter_;
            return;
        }
        if (motion_ != CompanionMotion::walking &&
            motion_ != CompanionMotion::running) {
            selectMotion(CompanionMotion::walking);
            movement_controller_.reset();
        }
    } else if (motion_ != CompanionMotion::running) {
        selectMotion(CompanionMotion::running);
        movement_controller_.reset();
    }

    const std::int32_t speed =
        motion_ == CompanionMotion::running
            ? profile_.running_speed
            : profile_.walking_speed;
    const MovementStepResult movement =
        movement_controller_.advance(
            ground,
            objects,
            judgement_,
            position_,
            owner_position,
            speed,
            dynamic_blockers,
            movementBlockerId());
    if (movement.moved) {
        direction_ = retailDirectionForVector(
            movement.position.x - position_.x,
            movement.position.y - position_.y);
        position_ = movement.position;
    }
    ++action_counter_;
}

void CompanionActor::updateCombatApproach(
    WorldPosition target_position,
    const GroundMap& ground,
    const ObjectMap& objects,
    const std::vector<MovementBlocker>*
        dynamic_blockers) {
    if (!valid() || attack_action_.active()) {
        return;
    }
    previous_position_ = position_;
    selectMotion(CompanionMotion::running);
    const MovementStepResult movement =
        movement_controller_.advance(
            ground,
            objects,
            judgement_,
            position_,
            target_position,
            profile_.running_speed,
            dynamic_blockers,
            movementBlockerId());
    if (movement.moved) {
        direction_ = retailDirectionForVector(
            movement.position.x - position_.x,
            movement.position.y - position_.y);
        position_ = movement.position;
    }
    ++action_counter_;
}

bool CompanionActor::beginAttack(
    std::int32_t target_character_number,
    WorldPosition target_position) {
    if (!valid() ||
        target_character_number < 0 ||
        attack_action_.active()) {
        return false;
    }
    direction_ = retailDirectionForVector(
        target_position.x - position_.x,
        target_position.y - position_.y);
    CompanionAttackAnimationTiming timing;
    if (!buildCompanionAttackAnimationTiming(
            visual_->animation(),
            direction_,
            timing) ||
        !attack_action_.start(
            profile_.attack_speed_rating,
            std::move(timing))) {
        return false;
    }
    combat_target_character_number_ =
        target_character_number;
    movement_controller_.reset();
    selectMotion(CompanionMotion::attacking);
    return true;
}

void CompanionActor::trackCombatTarget(
    std::int32_t target_character_number) {
    combat_target_character_number_ =
        target_character_number;
}

CompanionActorUpdate CompanionActor::updateAttack() {
    CompanionActorUpdate result;
    if (!attack_action_.active()) {
        return result;
    }
    previous_position_ = position_;
    const CompanionAttackActionEvent event =
        attack_action_.update();
    result.impact_due = event.impact_due;
    result.swing_sound_due = event.swing_sound_due;
    result.attack_completed = event.completed;
    if (event.completed) {
        selectMotion(CompanionMotion::idle);
    }
    return result;
}

bool CompanionActor::beginExplosion(
    WorldPosition destination) {
    if (!valid() || current_life_ <= 0 ||
        explosion_action_.active() ||
        presentation_action_ == 7 ||
        presentation_action_ == 9 ||
        presentation_action_ == 10) {
        return false;
    }
    pending_explosion_destination_ = destination;
    explosion_pending_ = true;
    if (attack_action_.active() ||
        presentation_action_ != 2) {
        return true;
    }
    return activatePendingExplosion();
}

bool CompanionActor::activatePendingExplosion() {
    if (!valid() || !explosion_pending_ ||
        current_life_ <= 0 ||
        explosion_action_.active() ||
        attack_action_.active() ||
        presentation_action_ != 2) {
        return false;
    }
    CompanionExplosionAnimationTiming timing;
    if (!buildCompanionExplosionAnimationTiming(
            visual_->animation(), timing) ||
        !explosion_action_.start(
            pending_explosion_destination_,
            std::move(timing))) {
        explosion_pending_ = false;
        pending_explosion_destination_ = {};
        return false;
    }
    explosion_pending_ = false;
    pending_explosion_destination_ = {};
    presentation_action_ = 10;
    presentation_counter_ = 0;
    action_lock_ = 1;
    combat_target_character_number_ = -1;
    attack_action_.cancel();
    movement_controller_.reset();
    selectMotion(CompanionMotion::exploding);
    return true;
}

CompanionExplosionUpdate
CompanionActor::updateExplosion() {
    CompanionExplosionUpdate result;
    if (!valid() || !explosion_action_.active()) {
        return result;
    }
    result.handled = true;
    previous_position_ = position_;
    const CompanionExplosionActionEvent event =
        explosion_action_.update();
    if (event.relocate_due) {
        position_ = explosion_action_.destination();
        previous_position_ = position_;
    }
    result.relocated = event.relocate_due;
    result.impact_due = event.impact_due;
    result.completed = event.completed;
    if (event.completed) {
        presentation_action_ = 2;
        presentation_counter_ = 0;
        action_lock_ = 0;
        selectMotion(CompanionMotion::idle);
    }
    return result;
}

void CompanionActor::leaveCombat() {
    combat_target_character_number_ = -1;
    attack_action_.cancel();
    explosion_action_.cancel();
    pending_explosion_destination_ = {};
    explosion_pending_ = false;
    movement_controller_.reset();
    selectMotion(CompanionMotion::idle);
}

CompanionPresentationUpdate
CompanionActor::updateDamagePresentation(
    const GroundMap& ground,
    const ObjectMap& objects,
    const std::vector<MovementBlocker>*
        dynamic_blockers) {
    CompanionPresentationUpdate result;
    if (!valid()) {
        return result;
    }
    previous_position_ = position_;
    if (presentation_action_ == 5) {
        result.handled = true;
        selectMotion(CompanionMotion::reacting);
        reaction_duration_ =
            std::max<std::int32_t>(
                reaction_duration_, 1);
        const std::int32_t count =
            std::max<std::int32_t>(
                frameCount(
                    visual_->animation(),
                    kHitChart,
                    direction_),
                1);
        presentation_animation_frame_ =
            presentation_counter_ * count /
            reaction_duration_;
        if (presentation_counter_ ==
            reaction_duration_ - 1) {
            presentation_animation_frame_ = count - 1;
        }
        if (reaction_stage_ == 2) {
            presentation_animation_frame_ = 0;
        }
        presentation_animation_frame_ =
            std::clamp(
                presentation_animation_frame_,
                std::int32_t{0},
                count - 1);

        if (!reaction_displacement_suppressed_ &&
            reaction_additive_ == 0) {
            const std::int32_t distance =
                (reaction_duration_ -
                 presentation_counter_) *
                kHitDisplacement /
                reaction_duration_;
            const WorldPosition destination{
                position_.x +
                    static_cast<std::int32_t>(
                        std::cos(reaction_angle_) *
                        distance),
                position_.y -
                    static_cast<std::int32_t>(
                        std::sin(reaction_angle_) *
                        distance),
            };
            position_ = advanceMovement(
                ground,
                objects,
                judgement_,
                position_,
                destination,
                distance,
                dynamic_blockers,
                movementBlockerId())
                            .position;
        }
        if (presentation_counter_ ==
            reaction_duration_ - 1) {
            presentation_action_ = 2;
            action_lock_ = 0;
            reaction_duration_ = 0;
            selectMotion(CompanionMotion::idle);
        } else {
            ++presentation_counter_;
        }
        if (reaction_additive_ != 0) {
            --reaction_additive_;
        }
        return result;
    }

    if (presentation_action_ == 6) {
        result.handled = true;
        selectMotion(CompanionMotion::defeated);
        if (presentation_counter_ == 0) {
            result.death_started = true;
            draw_opacity_ = 1000;
        }
        const std::int32_t count =
            std::max<std::int32_t>(
                frameCount(
                    visual_->animation(),
                    kDeathChart,
                    kSpecialDirection),
                1);
        presentation_animation_frame_ =
            std::min(
                presentation_counter_, count - 1);
        if (presentation_counter_ >=
            count + kDeathFadeUpdates - 1) {
            draw_opacity_ =
                std::max<std::int32_t>(
                    ((count - presentation_counter_) +
                     (kDeathFadeUpdates - 1)) *
                            1000 /
                            kDeathFadeUpdates +
                        1000,
                    0);
        }
        ++presentation_counter_;
        return result;
    }

    if (presentation_action_ == 8) {
        result.handled = true;
        selectMotion(CompanionMotion::reviving);
        const std::int32_t count =
            std::max<std::int32_t>(
                frameCount(
                    visual_->animation(),
                    kReviveChart,
                    kSpecialDirection),
                1);
        presentation_animation_frame_ =
            std::min(
                presentation_counter_, count - 1);
        if (presentation_counter_ == count - 1) {
            presentation_action_ = 2;
            action_lock_ = 0;
            combat_target_character_number_ = -1;
            selectMotion(CompanionMotion::idle);
            result.revive_completed = true;
        }
        ++presentation_counter_;
        return result;
    }
    return result;
}

CompanionDamageReceiverState
CompanionActor::damageReceiverState() const {
    CompanionDamageReceiverState state;
    state.character_number = characterNumber();
    state.position = position_;
    state.judgement = judgement_;
    state.current_life = current_life_;
    state.maximum_life = profile_.maximum_life;
    state.native_element = profile_.native_element;
    state.physical_defense =
        profile_.physical_defense;
    state.magical_defense =
        profile_.magical_defense;
    state.presentation_action = presentation_action_;
    state.presentation_counter =
        presentation_counter_;
    state.action_lock = action_lock_;
    state.reaction_duration = reaction_duration_;
    state.reaction_stage = reaction_stage_;
    state.reaction_motion =
        reaction_displacement_suppressed_;
    state.reaction_additive = reaction_additive_;
    state.reaction_angle = reaction_angle_;
    state.direction = direction_;
    state.event_number = event_number_;
    return state;
}

void CompanionActor::applyDamageReceiverState(
    const CompanionDamageReceiverState& state) {
    if (state.character_number != characterNumber()) {
        return;
    }
    current_life_ = state.current_life;
    presentation_action_ = state.presentation_action;
    presentation_counter_ = state.presentation_counter;
    action_lock_ = state.action_lock;
    reaction_duration_ = state.reaction_duration;
    reaction_stage_ = state.reaction_stage;
    reaction_displacement_suppressed_ =
        state.reaction_motion;
    reaction_additive_ = state.reaction_additive;
    reaction_angle_ = state.reaction_angle;
    direction_ = state.direction;
    event_number_ = state.event_number;
    if (presentation_action_ == 5 ||
        presentation_action_ == 6) {
        attack_action_.cancel();
        explosion_action_.cancel();
        movement_controller_.reset();
        combat_target_character_number_ = -1;
        selectMotion(
            presentation_action_ == 5
                ? CompanionMotion::reacting
                : CompanionMotion::defeated);
        if (presentation_action_ == 6) {
            pending_explosion_destination_ = {};
            explosion_pending_ = false;
        }
    }
}

void CompanionActor::beginDefeatedWait() {
    if (!valid()) {
        return;
    }
    current_life_ = 0;
    presentation_action_ = 6;
    presentation_counter_ = 5000;
    presentation_animation_frame_ = 0;
    action_lock_ = 1;
    draw_opacity_ = 0;
    attack_action_.cancel();
    explosion_action_.cancel();
    pending_explosion_destination_ = {};
    explosion_pending_ = false;
    movement_controller_.reset();
    combat_target_character_number_ = -1;
    selectMotion(CompanionMotion::defeated);
}

void CompanionActor::beginRevive(
    WorldPosition owner_position) {
    if (!valid()) {
        return;
    }
    position_ = owner_position;
    previous_position_ = owner_position;
    current_life_ = profile_.maximum_life;
    presentation_action_ = 8;
    presentation_counter_ = 0;
    presentation_animation_frame_ = 0;
    action_lock_ = 1;
    draw_opacity_ = 1000;
    attack_action_.cancel();
    explosion_action_.cancel();
    pending_explosion_destination_ = {};
    explosion_pending_ = false;
    movement_controller_.reset();
    combat_target_character_number_ = -1;
    selectMotion(CompanionMotion::reviving);
}

void CompanionActor::applyLevelProfile(
    const CompanionProfile& profile) {
    if (!valid() ||
        profile.type != profile_.type ||
        profile.resource_id != profile_.resource_id) {
        return;
    }
    profile_ = profile;
    current_life_ = profile_.maximum_life;
}

void CompanionActor::applyRuntimeProfile(
    const CompanionProfile& profile) {
    if (!valid() ||
        profile.type != profile_.type ||
        profile.resource_id != profile_.resource_id) {
        return;
    }
    profile_ = profile;
    current_life_ = std::clamp(
        current_life_, std::int32_t{0}, profile_.maximum_life);
}

bool CompanionActor::restoreLife(
    std::int32_t amount,
    std::int32_t maximum_percent) {
    if (!valid() || current_life_ <= 0 ||
        current_life_ >= profile_.maximum_life) {
        return false;
    }
    const std::int64_t restored =
        static_cast<std::int64_t>(current_life_) + amount +
        static_cast<std::int64_t>(maximum_percent) *
            profile_.maximum_life / 100;
    const std::int32_t next = static_cast<std::int32_t>(
        std::clamp<std::int64_t>(
            restored, 0, profile_.maximum_life));
    if (next == current_life_) {
        return false;
    }
    current_life_ = next;
    return true;
}

bool CompanionActor::valid() const {
    return visual_ != nullptr && owner_slot_ >= 0;
}

std::int32_t CompanionActor::characterNumber() const {
    return valid()
        ? kFirstCompanionCharacterNumber + owner_slot_
        : -1;
}

std::int32_t CompanionActor::movementBlockerId() const {
    return characterNumber();
}

const CompanionProfile& CompanionActor::profile() const {
    return profile_;
}

WorldPosition CompanionActor::position() const {
    return position_;
}

WorldPosition CompanionActor::renderPosition(
    double alpha) const {
    return interpolateWorldPosition(
        previous_position_, position_, alpha);
}

const ObjectBounds& CompanionActor::judgement() const {
    return judgement_;
}

std::int32_t CompanionActor::direction() const {
    return direction_;
}

CompanionMotion CompanionActor::motion() const {
    return motion_;
}

std::int32_t CompanionActor::animationChart() const {
    if (motion_ == CompanionMotion::exploding) {
        return explosion_action_.animationChart();
    }
    return chartForMotion(motion_);
}

std::int32_t CompanionActor::animationDirection() const {
    return motion_ == CompanionMotion::exploding ||
                   motion_ == CompanionMotion::defeated ||
                   motion_ == CompanionMotion::reviving
        ? kSpecialDirection
        : direction_;
}

std::int32_t CompanionActor::animationFrame() const {
    if (motion_ == CompanionMotion::attacking) {
        return attack_action_.animationFrame();
    }
    if (motion_ == CompanionMotion::exploding) {
        return explosion_action_.animationFrame();
    }
    if (motion_ == CompanionMotion::reacting ||
        motion_ == CompanionMotion::defeated ||
        motion_ == CompanionMotion::reviving) {
        return presentation_animation_frame_;
    }
    return action_counter_;
}

std::int32_t CompanionActor::drawOpacity() const {
    return draw_opacity_;
}

std::int32_t CompanionActor::presentationAction() const {
    return presentation_action_;
}

std::int32_t CompanionActor::currentLife() const {
    return current_life_;
}

std::int32_t CompanionActor::maximumLife() const {
    return profile_.maximum_life;
}

std::int32_t
CompanionActor::combatTargetCharacterNumber() const {
    return combat_target_character_number_;
}

bool CompanionActor::attackActive() const {
    return attack_action_.active();
}

bool CompanionActor::explosionPending() const {
    return explosion_pending_;
}

bool CompanionActor::explosionActive() const {
    return explosion_action_.active();
}

bool CompanionActor::partEnabled(std::size_t part) const {
    return valid() &&
           part < visual_->animation().maxPartCount();
}

std::int32_t CompanionActor::partRedStrength(
    std::size_t) const {
    return profile_.red_strength;
}

std::int32_t CompanionActor::partGreenStrength(
    std::size_t) const {
    return profile_.green_strength;
}

std::int32_t CompanionActor::partBlueStrength(
    std::size_t) const {
    return profile_.blue_strength;
}

const gapi::NjpImage& CompanionActor::patterns() const {
    return visual_->patterns();
}

const gapi::NjpImage&
CompanionActor::shadowPatterns() const {
    return visual_->shadowPatterns();
}

const gapi::CafAnimation& CompanionActor::animation() const {
    return visual_->animation();
}

bool CompanionActor::visible() const {
    return valid() && draw_opacity_ > 0;
}

bool CompanionActor::judgementEnabled() const {
    return valid() && current_life_ > 0;
}

void CompanionActor::selectMotion(
    CompanionMotion motion) {
    if (motion_ == motion) {
        return;
    }
    motion_ = motion;
    action_counter_ = 0;
}

}  // namespace osf
