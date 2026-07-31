#include "companion_actor.hpp"

#include "actor_direction.hpp"
#include "resources/character_visual_resource.hpp"

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

std::int32_t chartForMotion(CompanionMotion motion) {
    switch (motion) {
    case CompanionMotion::walking:
        return 1;
    case CompanionMotion::running:
        return 2;
    case CompanionMotion::attacking:
        return 5;
    case CompanionMotion::idle:
    default:
        return 0;
    }
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
    combat_target_character_number_ = -1;
    movement_controller_.reset();
    attack_action_.cancel();
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
    selectMotion(CompanionMotion::idle);
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

void CompanionActor::leaveCombat() {
    combat_target_character_number_ = -1;
    attack_action_.cancel();
    movement_controller_.reset();
    selectMotion(CompanionMotion::idle);
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
    return chartForMotion(motion_);
}

std::int32_t CompanionActor::animationFrame() const {
    if (motion_ == CompanionMotion::attacking) {
        return attack_action_.animationFrame();
    }
    return action_counter_;
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
    return valid();
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
