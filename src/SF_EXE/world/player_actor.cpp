#include "player_actor.hpp"
#include "movement_controller.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <utility>

namespace osf {
namespace {

constexpr std::array<double, 10> kSpeedFactors{{
    2.0 / 3.0,
    3.0 / 4.0,
    4.0 / 5.0,
    5.0 / 6.0,
    6.0 / 7.0,
    1.0,
    7.0 / 6.0,
    6.0 / 5.0,
    5.0 / 4.0,
    4.0 / 3.0,
}};

double speedFactor(std::int32_t tier) {
    return kSpeedFactors[
        static_cast<std::size_t>(
            std::clamp<std::int32_t>(tier, 0, 9))];
}

std::int32_t walkingSpeedForTier(std::int32_t tier) {
    // FUN_00450080 multiplies the tier factor by the retail base speed 20.0
    // before truncating it into the movement controller's integer step.
    return static_cast<std::int32_t>(
        speedFactor(tier) * 20.0);
}

std::int32_t animationFrameForSpeed(
    std::int32_t counter,
    std::int32_t tier) {
    return static_cast<std::int32_t>(
        static_cast<double>(counter) *
        speedFactor(tier));
}

std::int32_t frameCount(
    const gapi::CafAnimation* animation,
    std::int32_t chart,
    std::int32_t direction) {
    if (!animation || chart < 0 || direction < 0 ||
        direction >= 9 ||
        static_cast<std::size_t>(chart) >=
            animation->charts().size()) {
        return 0;
    }
    return animation->charts()[
               static_cast<std::size_t>(chart)]
        .directions[
            static_cast<std::size_t>(direction)]
        .frame_count;
}

}  // namespace

void PlayerActor::reset(
    WorldPosition position,
    std::int32_t direction,
    std::int32_t walking_speed_tier) {
    position_ = position;
    previous_position_ = position;
    destination_ = position;
    direction_ = direction;
    walking_speed_tier_ =
        std::clamp<std::int32_t>(walking_speed_tier, 0, 9);
    walking_speed_ =
        walkingSpeedForTier(walking_speed_tier_);
    running_speed_ = walking_speed_ * 2;
    action_counter_ = 0;
    animation_chart_ = 0;
    animation_frame_ = 0;
    movement_pace_ = MovementPace::walk;
    motion_ = PlayerMotion::idle;
    previous_action_ = PlayerMotion::idle;
    attack_controller_.cancel();
    pending_attack_event_ = {};
    spell_controller_.cancel();
    pending_spell_event_ = {};
    pending_footstep_sample_ = -1;
    pending_death_voice_request_ = false;
    respawn_requested_ = false;
    pending_respawn_request_ = false;
    movement_controller_.reset();
    damage_presentation_ = {};
}

void PlayerActor::clear() {
    reset({}, 0);
}

void PlayerActor::relocate(
    WorldPosition position,
    std::int32_t direction) {
    position_ = position;
    previous_position_ = position;
    destination_ = position;
    direction_ = direction;
    action_counter_ = 0;
    animation_chart_ = 0;
    animation_frame_ = 0;
    motion_ = PlayerMotion::idle;
    previous_action_ = PlayerMotion::idle;
    attack_controller_.cancel();
    pending_attack_event_ = {};
    spell_controller_.cancel();
    pending_spell_event_ = {};
    pending_footstep_sample_ = -1;
    pending_death_voice_request_ = false;
    respawn_requested_ = false;
    pending_respawn_request_ = false;
    movement_controller_.reset();
    damage_presentation_ = {};
}

void PlayerActor::moveTo(WorldPosition destination) {
    if (attack_controller_.active() ||
        spell_controller_.active() ||
        damage_presentation_.action_lock != 0) {
        return;
    }
    destination_ = destination;
    motion_ =
        movement_pace_ == MovementPace::run
            ? PlayerMotion::running
            : PlayerMotion::walking;
}

void PlayerActor::followTo(WorldPosition destination) {
    if (attack_controller_.active() ||
        spell_controller_.active() ||
        damage_presentation_.action_lock != 0) {
        return;
    }
    destination_ = destination;
    motion_ =
        movement_pace_ == MovementPace::run
            ? PlayerMotion::running
            : PlayerMotion::walking;
}

void PlayerActor::cancelMovement() {
    if (attack_controller_.active() ||
        spell_controller_.active() ||
        damage_presentation_.action_lock != 0) {
        return;
    }
    destination_ = position_;
    motion_ = PlayerMotion::idle;
    movement_controller_.reset();
}

void PlayerActor::faceToward(WorldPosition position) {
    if (position.x == position_.x &&
        position.y == position_.y) {
        return;
    }
    direction_ = retailDirectionForVector(
        position.x - position_.x,
        position.y - position_.y);
}

bool PlayerActor::beginAttack(
    PlayerAttackAction action,
    std::int32_t target_id,
    std::int32_t attack_speed_tier,
    const gapi::CafAnimation& animation) {
    if (attack_controller_.active() ||
        spell_controller_.active() ||
        damage_presentation_.action_lock != 0) {
        return false;
    }
    PlayerAttackAnimationTiming timing;
    if (!buildPlayerAttackAnimationTiming(
            animation,
            action,
            direction_,
            timing)) {
        return false;
    }
    destination_ = position_;
    movement_controller_.reset();
    if (!attack_controller_.start(
            action,
            target_id,
            attack_speed_tier,
            std::move(timing),
            &pending_attack_event_)) {
        return false;
    }
    action_counter_ = 0;
    animation_chart_ =
        attack_controller_.animationChart();
    animation_frame_ =
        attack_controller_.animationFrame();
    motion_ = PlayerMotion::attacking;
    previous_action_ = PlayerMotion::attacking;
    return true;
}

bool PlayerActor::beginComboAttack(
    PlayerComboAttackKind kind,
    std::int32_t attack_speed_tier,
    const gapi::CafAnimation& animation) {
    if (attack_controller_.active() ||
        spell_controller_.active() ||
        damage_presentation_.action_lock != 0) {
        return false;
    }
    destination_ = position_;
    movement_controller_.reset();
    if (!attack_controller_.startCombo(
            kind,
            attack_speed_tier,
            animation,
            direction_,
            &pending_attack_event_)) {
        return false;
    }
    action_counter_ = 0;
    animation_chart_ =
        attack_controller_.animationChart();
    animation_frame_ =
        attack_controller_.animationFrame();
    motion_ = PlayerMotion::attacking;
    previous_action_ = PlayerMotion::attacking;
    return true;
}

bool PlayerActor::beginSpellCast(
    PlayerSpellAction action,
    std::int32_t spell,
    std::int32_t target_character_number,
    WorldPosition aim_position,
    PlayerSpellAnimationVariant animation_variant,
    std::int32_t speed_tier,
    const TableData* speed_table,
    const gapi::CafAnimation& animation) {
    if (attack_controller_.active() ||
        spell_controller_.active() ||
        damage_presentation_.action_lock != 0) {
        return false;
    }
    PlayerSpellAnimationTiming timing;
    if (!buildPlayerSpellAnimationTiming(
        animation,
        action,
        animation_variant,
        direction_,
            timing)) {
        return false;
    }
    destination_ = position_;
    movement_controller_.reset();
    if (!spell_controller_.start(
            action,
            spell,
            target_character_number,
            aim_position.x,
            aim_position.y,
            speed_tier,
            speed_table,
            std::move(timing),
            &pending_spell_event_)) {
        return false;
    }
    action_counter_ = 0;
    animation_chart_ =
        spell_controller_.animationChart();
    animation_frame_ =
        spell_controller_.animationFrame();
    motion_ = PlayerMotion::casting;
    previous_action_ = PlayerMotion::casting;
    return true;
}

void PlayerActor::toggleMovementPace() {
    movement_pace_ =
        movement_pace_ == MovementPace::walk
            ? MovementPace::run
            : MovementPace::walk;
    if (motion_ == PlayerMotion::walking ||
        motion_ == PlayerMotion::running) {
        motion_ =
            movement_pace_ == MovementPace::run
                ? PlayerMotion::running
                : PlayerMotion::walking;
    }
}

void PlayerActor::setWalkingSpeedTier(std::int32_t tier) {
    walking_speed_tier_ = std::clamp<std::int32_t>(tier, 0, 9);
    walking_speed_ = walkingSpeedForTier(walking_speed_tier_);
    running_speed_ = walking_speed_ * 2;
}

void PlayerActor::setMovementPace(MovementPace pace) {
    movement_pace_ = pace;
    if (motion_ == PlayerMotion::walking ||
        motion_ == PlayerMotion::running) {
        motion_ =
            movement_pace_ == MovementPace::run
                ? PlayerMotion::running
                : PlayerMotion::walking;
    }
}

void PlayerActor::update(
    const GroundMap& ground,
    const ObjectMap& objects,
    const std::vector<MovementBlocker>* dynamic_blockers,
    std::int32_t attack_speed_tier,
    const gapi::CafAnimation* animation,
    const TableData* spell_speed_table) {
    previous_position_ = position_;
    pending_footstep_sample_ = -1;
    if (damage_presentation_.action == 4) {
        constexpr std::int32_t kHitChart = 3;
        constexpr std::int32_t kHitDisplacement = 120;
        damage_presentation_.action_lock = 1;
        damage_presentation_.reaction_duration =
            std::max<std::int32_t>(
                damage_presentation_.reaction_duration,
                1);
        motion_ = PlayerMotion::reacting;
        animation_chart_ = kHitChart;
        const std::int32_t count =
            std::max<std::int32_t>(
                frameCount(
                    animation,
                    animation_chart_,
                    direction_),
                1);
        animation_frame_ =
            damage_presentation_.counter * count /
            damage_presentation_.reaction_duration;
        if (damage_presentation_.counter ==
            damage_presentation_.reaction_duration - 1) {
            animation_frame_ = count - 1;
        }
        animation_frame_ = std::clamp<std::int32_t>(
            animation_frame_, 0, count - 1);
        if (damage_presentation_.reaction_stage == 2) {
            animation_frame_ = 0;
        }

        if (!damage_presentation_.suppress_displacement &&
            damage_presentation_.reaction_additive == 0) {
            const std::int32_t distance =
                (damage_presentation_.reaction_duration -
                 damage_presentation_.counter) *
                kHitDisplacement /
                damage_presentation_.reaction_duration;
            const WorldPosition destination{
                position_.x +
                    static_cast<std::int32_t>(
                        std::cos(
                            damage_presentation_
                                .reaction_angle) *
                        distance),
                position_.y -
                    static_cast<std::int32_t>(
                        std::sin(
                            damage_presentation_
                                .reaction_angle) *
                        distance),
            };
            position_ = advanceMovement(
                ground,
                objects,
                judgement_,
                position_,
                destination,
                distance,
                dynamic_blockers)
                            .position;
        }
        if (damage_presentation_.counter ==
            damage_presentation_.reaction_duration - 1) {
            damage_presentation_.action = 1;
            damage_presentation_.action_lock = 0;
            damage_presentation_.reaction_duration = 0;
            motion_ = PlayerMotion::idle;
            action_counter_ = 0;
        } else {
            ++damage_presentation_.counter;
        }
        if (damage_presentation_.reaction_additive != 0) {
            --damage_presentation_.reaction_additive;
        }
        return;
    }
    if (damage_presentation_.action == 5) {
        constexpr std::int32_t kDeathChart = 4;
        constexpr std::int32_t kDeathDirection = 8;
        if (damage_presentation_.counter == 0) {
            // FUN_00435b60 plays the gender-specific voice before advancing
            // the first frame of the locked death presentation.
            pending_death_voice_request_ = true;
        }
        damage_presentation_.action_lock = 1;
        motion_ = PlayerMotion::defeated;
        animation_chart_ = kDeathChart;
        if (frameCount(
                animation,
                animation_chart_,
                kDeathDirection) > 0) {
            direction_ = kDeathDirection;
        }
        const std::int32_t count =
            std::max<std::int32_t>(
                frameCount(
                    animation,
                    animation_chart_,
                    direction_),
                1);
        animation_frame_ = std::min(
            damage_presentation_.counter,
            count - 1);
        ++damage_presentation_.counter;
        // FUN_00435b60 holds the final death frame for 120 updates before
        // requesting a scenario-entry transition with revival enabled.
        if (!respawn_requested_ &&
            damage_presentation_.counter >= count + 120) {
            respawn_requested_ = true;
            pending_respawn_request_ = true;
        }
        return;
    }
    if (attack_controller_.active()) {
        pending_attack_event_ =
            attack_controller_.update(attack_speed_tier);
        if (pending_attack_event_.lunge_distance > 0) {
            const double angle =
                retailAngleForDirection(direction_);
            const WorldPosition lunge_destination{
                position_.x + static_cast<std::int32_t>(
                    std::cos(angle) *
                    pending_attack_event_.lunge_distance),
                position_.y - static_cast<std::int32_t>(
                    std::sin(angle) *
                    pending_attack_event_.lunge_distance),
            };
            position_ = advanceMovement(
                ground,
                objects,
                judgement_,
                position_,
                lunge_destination,
                pending_attack_event_.lunge_distance,
                dynamic_blockers)
                            .position;
        }
        animation_chart_ =
            attack_controller_.animationChart();
        animation_frame_ =
            attack_controller_.animationFrame();
        if (!attack_controller_.active()) {
            motion_ = PlayerMotion::idle;
        }
        return;
    }
    if (spell_controller_.active()) {
        pending_spell_event_ =
            spell_controller_.update(
                attack_speed_tier,
                spell_speed_table);
        animation_chart_ =
            spell_controller_.animationChart();
        animation_frame_ =
            spell_controller_.animationFrame();
        if (!spell_controller_.active()) {
            motion_ = PlayerMotion::idle;
        }
        return;
    }
    if (motion_ == PlayerMotion::idle) {
        if (previous_action_ != PlayerMotion::idle) {
            action_counter_ = 0;
        }
        animation_chart_ = 0;
        animation_frame_ = action_counter_;
        ++action_counter_;
        previous_action_ = PlayerMotion::idle;
        return;
    }

    const PlayerMotion moving_action = motion_;
    if (previous_action_ != moving_action) {
        action_counter_ = 0;
    } else {
        ++action_counter_;
    }
    animation_chart_ =
        moving_action == PlayerMotion::running ? 2 : 1;
    animation_frame_ = animationFrameForSpeed(
        action_counter_, walking_speed_tier_);
    previous_action_ = moving_action;
    const std::int32_t footstep_interval =
        moving_action == PlayerMotion::running ? 8 : 12;
    if (action_counter_ % footstep_interval == 0) {
        // FUN_004351f0 and FUN_00435530 both play Voice00 sample zero.
        pending_footstep_sample_ = 0;
    }

    if (position_.x == destination_.x &&
        position_.y == destination_.y) {
        motion_ = PlayerMotion::idle;
        return;
    }

    direction_ = retailDirectionForVector(
        destination_.x - position_.x,
        destination_.y - position_.y);
    const MovementStepResult movement =
        movement_controller_.advance(
            ground,
            objects,
            judgement_,
            position_,
            destination_,
            moving_action == PlayerMotion::running
                ? running_speed_
                : walking_speed_,
            dynamic_blockers);
    if (movement.moved) {
        direction_ = retailDirectionForVector(
            movement.position.x - position_.x,
            movement.position.y - position_.y);
    }
    position_ = movement.position;
    if (movement.moved || movement.controller_active) {
        return;
    }
    motion_ = PlayerMotion::idle;
}

WorldPosition PlayerActor::position() const {
    return position_;
}

WorldPosition PlayerActor::renderPosition(double alpha) const {
    return interpolateWorldPosition(
        previous_position_, position_, alpha);
}

WorldPosition PlayerActor::destination() const {
    return destination_;
}

const ObjectBounds& PlayerActor::judgement() const {
    return judgement_;
}

std::int32_t PlayerActor::direction() const {
    return direction_;
}

std::int32_t PlayerActor::walkingSpeedTier() const {
    return walking_speed_tier_;
}

std::int32_t PlayerActor::walkingSpeed() const {
    return walking_speed_;
}

std::int32_t PlayerActor::runningSpeed() const {
    return running_speed_;
}

MovementPace PlayerActor::movementPace() const {
    return movement_pace_;
}

PlayerMotion PlayerActor::motion() const {
    return motion_;
}

bool PlayerActor::actionLocked() const {
    return attack_controller_.active() ||
           spell_controller_.active() ||
           damage_presentation_.action_lock != 0;
}

bool PlayerActor::attackActive() const {
    return attack_controller_.active();
}

std::int32_t PlayerActor::attackTargetId() const {
    return attack_controller_.active()
        ? attack_controller_.targetId()
        : -1;
}

PlayerAttackActionEvent PlayerActor::takeAttackEvent() {
    PlayerAttackActionEvent event = pending_attack_event_;
    pending_attack_event_ = {};
    return event;
}

bool PlayerActor::spellActive() const {
    return spell_controller_.active();
}

std::int32_t
PlayerActor::spellTargetCharacterNumber() const {
    return spell_controller_.active()
        ? spell_controller_.targetCharacterNumber()
        : -1;
}

PlayerSpellActionEvent PlayerActor::takeSpellEvent() {
    PlayerSpellActionEvent event = pending_spell_event_;
    pending_spell_event_ = {};
    return event;
}

std::int32_t PlayerActor::takeFootstepSample() {
    const std::int32_t sample = pending_footstep_sample_;
    pending_footstep_sample_ = -1;
    return sample;
}

bool PlayerActor::takeDeathVoiceRequest() {
    const bool requested = pending_death_voice_request_;
    pending_death_voice_request_ = false;
    return requested;
}

bool PlayerActor::takeRespawnRequest() {
    const bool requested = pending_respawn_request_;
    pending_respawn_request_ = false;
    return requested;
}

PlayerDamagePresentation
PlayerActor::damagePresentation() const {
    PlayerDamagePresentation presentation =
        damage_presentation_;
    if (presentation.action_lock != 0) {
        return presentation;
    }
    if (attack_controller_.active()) {
        presentation.action =
            static_cast<std::int32_t>(
                attack_controller_.action());
    } else if (spell_controller_.active()) {
        presentation.action =
            static_cast<std::int32_t>(
                spell_controller_.action());
    } else if (motion_ == PlayerMotion::walking) {
        presentation.action = 2;
    } else if (motion_ == PlayerMotion::running) {
        presentation.action = 3;
    } else {
        presentation.action = 1;
    }
    presentation.counter = action_counter_;
    presentation.direction = direction_;
    return presentation;
}

void PlayerActor::applyDamagePresentation(
    const PlayerDamagePresentation& presentation) {
    damage_presentation_ = presentation;
    direction_ = presentation.direction;
    if (presentation.action != 4 &&
        presentation.action != 5) {
        return;
    }
    destination_ = position_;
    movement_controller_.reset();
    attack_controller_.cancel();
    pending_attack_event_ = {};
    spell_controller_.cancel();
    pending_spell_event_ = {};
    respawn_requested_ = false;
    pending_respawn_request_ = false;
    action_counter_ = 0;
    motion_ =
        presentation.action == 4
            ? PlayerMotion::reacting
            : PlayerMotion::defeated;
}

std::int32_t PlayerActor::animationChart() const {
    return animation_chart_;
}

std::int32_t PlayerActor::animationFrame() const {
    return animation_frame_;
}

}  // namespace osf
