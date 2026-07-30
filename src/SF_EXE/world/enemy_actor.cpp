#include "enemy_actor.hpp"

#include "actor_direction.hpp"
#include "enemy_ai_evaluator.hpp"
#include "enemy_presentation_audio.hpp"
#include "libs/RKC_RPG_AICONTROL/rkc_rpg_aicontrol.hpp"
#include "resources/character_visual_resource.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

namespace osf {
namespace {

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

template <typename Value>
void copyParts(
    std::vector<Value>& destination,
    const std::vector<Value>& source) {
    std::copy_n(
        source.begin(),
        std::min(destination.size(), source.size()),
        destination.begin());
}

constexpr std::int32_t kIdlePresentationAction = 7;
constexpr std::int32_t kHitPresentationAction = 10;
constexpr std::int32_t kDeathPresentationAction = 11;
constexpr std::int32_t kHitAnimationChart = 2;
constexpr std::int32_t kDeathAnimationChart = 3;
constexpr std::int32_t kDeathDirection = 8;
constexpr std::int32_t kHitDisplacement = 120;
constexpr std::int32_t kDeathFadeUpdates = 120;
constexpr std::int32_t kDeathEffectNumber = 21010;
constexpr std::int32_t kWalkPresentationAction = 8;
constexpr std::int32_t kWalkAnimationChart = 1;

const gapi::CafDirection* animationDirection(
    const CharacterVisualResource* visual,
    std::int32_t chart,
    std::int32_t direction) {
    if (!visual || chart < 0 || direction < 0 ||
        direction >= 9 ||
        static_cast<std::size_t>(chart) >=
            visual->animation().charts().size()) {
        return nullptr;
    }
    return &visual->animation()
                .charts()[static_cast<std::size_t>(chart)]
                .directions[
                    static_cast<std::size_t>(direction)];
}

void appendMarkerAudio(
    const gapi::CafDirection& direction,
    std::int32_t frame,
    std::int32_t resource_id,
    std::int32_t chart,
    std::vector<std::int32_t>& samples) {
    if (frame < 0 || direction.parts.empty() ||
        static_cast<std::size_t>(frame) >=
            direction.parts.front().size()) {
        return;
    }
    const std::uint16_t status =
        static_cast<std::uint16_t>(
            direction.parts.front()[
                static_cast<std::size_t>(frame)]
                .status);
    constexpr std::uint16_t first_marker = 0x400u;
    for (std::int32_t slot = 0; slot < 3; ++slot) {
        if ((status &
             (first_marker <<
              static_cast<std::uint32_t>(slot))) == 0) {
            continue;
        }
        const std::int32_t sample =
            retailEnemyPresentationSample(
                resource_id, chart, slot);
        if (sample >= 0) {
            samples.push_back(sample);
        }
    }
}

WorldPosition displacedPosition(
    WorldPosition position,
    double angle,
    std::int32_t distance) {
    return {
        position.x +
            static_cast<std::int32_t>(
                std::cos(angle) *
                static_cast<double>(distance)),
        position.y -
            static_cast<std::int32_t>(
                std::sin(angle) *
                static_cast<double>(distance)),
    };
}

CombatEffectSpawnRequest deathEffect(
    const EnemyActor& enemy,
    std::int32_t direction) {
    CombatEffectSpawnRequest request;
    request.valid = true;
    request.effect_number = kDeathEffectNumber;
    request.owner_kind = 4;
    request.source_character_number =
        enemy.characterNumber();
    request.target_kind = 0;
    request.target_identifier = 0;
    request.has_source_judgement = true;
    request.source_judgement = enemy.judgement();
    request.packet_kind = direction;
    request.instance_identifier = -1;
    request.constructor_value_21 = 200;
    return request;
}

}  // namespace

bool EnemyActor::initialize(
    const ScenarioEnemy& enemy,
    const CharacterVisualResource* visual,
    const AiControlList& ai_control,
    std::int32_t ai_control_index,
    std::string* error) {
    clear();
    if (enemy.resource_id >= 0 && !visual) {
        setError(error, "The enemy animation resource is missing.");
        return false;
    }
    if (ai_control_index < 0 ||
        ai_control.name() != enemy.ai_control_name) {
        setError(error, "The enemy AI-control list is invalid.");
        return false;
    }
    if (!state_.initialize(enemy.initial_state_values)) {
        setError(
            error,
            "The enemy does not contain its three retail state values.");
        return false;
    }
    if (visual && visual->animation().charts().empty()) {
        setError(error, "The enemy animation contains no CAF charts.");
        clear();
        return false;
    }

    visual_ = visual;
    id_ = enemy.id;
    resource_id_ = enemy.resource_id;
    name_ = enemy.name;
    name_color_ = enemy.name_color;
    label_height_ = enemy.label_height;
    position_ = {enemy.world_x, enemy.world_y};
    previous_position_ = position_;
    spawn_position_ = position_;
    walk_point_ = position_;
    judgement_ = {
        enemy.judgement_left,
        enemy.judgement_top,
        enemy.judgement_right,
        enemy.judgement_bottom,
    };
    direction_ = enemy.direction;
    ai_control_name_ = enemy.ai_control_name;
    ai_control_ = &ai_control;
    ai_control_index_ = ai_control_index;
    patrol_bounds_ = {
        enemy.patrol_left,
        enemy.patrol_top,
        enemy.patrol_right,
        enemy.patrol_bottom,
    };
    current_life_ = enemy.maximum_life;
    maximum_life_ = enemy.maximum_life;
    native_element_ = enemy.native_element;
    physical_defense_ = enemy.physical_defense;
    physical_evasion_ = enemy.physical_evasion;
    magical_defense_ = enemy.magical_defense;
    magical_evasion_ = enemy.magical_evasion;
    experience_reward_ = enemy.experience_reward;
    loot_table_row_ = enemy.loot_table_row;
    gold_drop_chance_ = enemy.gold_drop_chance;
    gold_minimum_ = enemy.gold_minimum;
    gold_maximum_ = enemy.gold_maximum;
    reaction_chance_defense_ =
        enemy.reaction_chance_defense;
    reaction_duration_defense_ =
        enemy.reaction_duration_defense;
    always_suppress_reaction_displacement_ =
        enemy.always_suppress_reaction_displacement;
    movement_speed_scale_ =
        enemy.movement_speed_scale;
    presentation_profile_ = enemy.presentation;
    ai_action_.reset();
    presentation_.reset();
    movement_destination_.reset();
    movement_controller_.reset();

    const std::size_t part_count =
        visual
            ? visual->animation().maxPartCount()
            : 0;
    part_visibility_.assign(part_count, 1);
    red_strength_.assign(part_count, 1000);
    green_strength_.assign(part_count, 1000);
    blue_strength_.assign(part_count, 1000);
    if (!enemy.part_visibility.empty()) {
        copyParts(
            part_visibility_, enemy.part_visibility);
        copyParts(red_strength_, enemy.red_strength);
        copyParts(green_strength_, enemy.green_strength);
        copyParts(blue_strength_, enemy.blue_strength);
    }
    if (error) {
        error->clear();
    }
    return true;
}

void EnemyActor::clear() {
    id_ = -1;
    resource_id_ = -1;
    name_.clear();
    name_color_ = 0;
    label_height_ = 0;
    position_ = {};
    previous_position_ = {};
    judgement_ = {};
    direction_ = 0;
    animation_chart_ = 0;
    animation_frame_ = 0;
    action_counter_ = 0;
    ai_control_name_.clear();
    ai_control_ = nullptr;
    ai_control_index_ = -1;
    patrol_bounds_ = {};
    current_life_ = 0;
    maximum_life_ = 0;
    native_element_ = 0;
    physical_defense_ = 0;
    physical_evasion_ = 0;
    magical_defense_ = 0;
    magical_evasion_ = 0;
    experience_reward_ = 0;
    loot_table_row_ = -1;
    gold_drop_chance_ = 0;
    gold_minimum_ = 0;
    gold_maximum_ = 0;
    reaction_chance_defense_ = 0;
    reaction_duration_defense_ = 0;
    always_suppress_reaction_displacement_ = false;
    presentation_action_ = 7;
    action_lock_ = 0;
    reaction_duration_ = 0;
    reaction_stage_ = 0;
    reaction_displacement_suppressed_ = false;
    reaction_additive_ = 0;
    reaction_angle_ = 0.0;
    event_number_ = 0;
    attributed_damage_.fill(0);
    death_counter_ = 0;
    defeated_by_effect_ = false;
    defeat_source_character_number_ = -1;
    draw_strength_ = 1000;
    expired_ = false;
    movement_speed_scale_ = 0;
    spawn_position_ = {};
    walk_point_ = {};
    ai_action_.reset();
    presentation_.reset();
    movement_destination_.reset();
    movement_controller_.reset();
    movement_speed_ = 0;
    movement_action_counter_ = 0;
    presentation_profile_ = {};
    state_.clear();
    part_visibility_.clear();
    red_strength_.clear();
    green_strength_.clear();
    blue_strength_.clear();
    visual_ = nullptr;
}

EnemyActorUpdate EnemyActor::update(
    const EnemyActorUpdateContext& context) {
    EnemyActorUpdate result;
    previous_position_ = position_;
    if (expired_) {
        result.expired = true;
        return result;
    }

    if (presentation_action_ == kHitPresentationAction) {
        action_lock_ = 1;
        reaction_duration_ =
            std::max<std::int32_t>(
                reaction_duration_, 1);
        animation_chart_ = kHitAnimationChart;
        const gapi::CafDirection* animation =
            animationDirection(
                visual_,
                animation_chart_,
                direction_);
        const std::int32_t frame_count =
            animation
                ? animation->frame_count
                : 0;
        if (animation && frame_count > 0) {
            appendMarkerAudio(
                *animation,
                action_counter_ % frame_count,
                resource_id_,
                animation_chart_,
                result.audio_samples);
            animation_frame_ =
                action_counter_ * frame_count /
                reaction_duration_;
            if (action_counter_ ==
                reaction_duration_ - 1) {
                animation_frame_ = frame_count - 1;
            }
            animation_frame_ = std::clamp(
                animation_frame_, 0, frame_count - 1);
        } else {
            animation_frame_ = 0;
            reaction_duration_ = 1;
        }
        if (reaction_stage_ == 2) {
            animation_frame_ = 0;
        }

        // The retail field is true when authored displacement is
        // suppressed. A false value applies the diminishing
        // 120-unit impulse away from the impact origin.
        if (!reaction_displacement_suppressed_ &&
            reaction_additive_ == 0) {
            const std::int32_t distance =
                (reaction_duration_ - action_counter_) *
                kHitDisplacement /
                reaction_duration_;
            if (distance != 0) {
                const WorldPosition destination =
                    displacedPosition(
                        position_,
                        reaction_angle_,
                        distance);
                if (context.ground && context.objects) {
                    position_ = advanceMovement(
                        *context.ground,
                        *context.objects,
                        judgement_,
                        position_,
                        destination,
                        distance,
                        context.dynamic_blockers,
                        movementBlockerId())
                                    .position;
                }
            }
        }

        if (action_counter_ ==
            reaction_duration_ - 1) {
            action_lock_ = 0;
            presentation_action_ =
                kIdlePresentationAction;
            if (event_number_ == -1) {
                event_number_ = 16;
            }
        }
        ++action_counter_;
        if (reaction_additive_ != 0) {
            --reaction_additive_;
        }
        return result;
    }

    if (presentation_action_ == kDeathPresentationAction) {
        action_lock_ = 1;
        animation_chart_ = kDeathAnimationChart;
        if (action_counter_ == 0) {
            draw_strength_ = 1000;
            result.death_started = true;
            result.effect_spawn = deathEffect(*this, 0);
        }

        const gapi::CafDirection* death_direction =
            animationDirection(
                visual_,
                animation_chart_,
                kDeathDirection);
        if (death_direction &&
            death_direction->frame_count > 0) {
            direction_ = kDeathDirection;
        }
        const gapi::CafDirection* animation =
            animationDirection(
                visual_,
                animation_chart_,
                direction_);
        const std::int32_t frame_count =
            animation && animation->frame_count > 0
                ? animation->frame_count
                : 1;
        if (animation) {
            appendMarkerAudio(
                *animation,
                action_counter_,
                resource_id_,
                animation_chart_,
                result.audio_samples);
        }
        if (action_counter_ == 1 && visual_) {
            const std::int32_t sample =
                retailEnemyDeathSample(resource_id_);
            if (sample >= 0) {
                result.audio_samples.push_back(sample);
            }
        }

        animation_frame_ = std::min(
            action_counter_, frame_count - 1);
        if (action_counter_ >= frame_count - 1) {
            draw_strength_ = std::max<std::int32_t>(
                ((frame_count - action_counter_) +
                 (kDeathFadeUpdates - 1)) *
                    1000 /
                    kDeathFadeUpdates,
                0);
        }
        if (action_counter_ >=
            frame_count + kDeathFadeUpdates - 1) {
            expired_ = true;
            result.expired = true;
            return result;
        }
        ++action_counter_;
        return result;
    }

    // UpdateEnemy evaluates and promotes a native AID action only while
    // the direct/effect presentation lock is clear. Once selected, that
    // presentation keeps control until it publishes its completion event.
    bool run_presentation =
        presentation_.presentationAction() >= 1 &&
        presentation_.presentationAction() <= 6;
    if (!run_presentation && ai_control_ &&
        context.random) {
        const EnemyAiSelection selected =
            evaluateEnemyAiEvent(
                *ai_control_,
                event_number_,
                {
                    current_life_,
                    maximum_life_,
                    context.target_in_range,
                },
                *context.random);
        if (selected.selected) {
            ai_action_.select(selected.action);
            event_number_ = -1;
        }

        EnemyAiActionContext action_context;
        action_context.spawn_position =
            spawn_position_;
        action_context.patrol_bounds =
            patrol_bounds_;
        action_context.movement_speed_scale =
            movement_speed_scale_;
        action_context.presentation_action =
            presentation_action_;
        action_context.walk_point = walk_point_;
        action_context.walk_point_speed =
            ai_control_->walkPointSpeed();
        action_context.target_in_range =
            context.target_in_range;
        action_context.default_target =
            context.default_target;
        const EnemyAiActionUpdate action =
            ai_action_.update(action_context);
        if (action.handled) {
            event_number_ = action.event_number;
        }
        if (action.clear_current_presentation) {
            presentation_.reset();
            movement_destination_.reset();
            movement_controller_.reset();
            movement_speed_ = 0;
        }
        if (action.requested_presentation_action >= 1 &&
            action.requested_presentation_action <= 6) {
            presentation_.select(
                action.requested_presentation_action);
            run_presentation = true;
        } else if (
            action.requested_presentation_action ==
                kIdlePresentationAction ||
            action.requested_presentation_action ==
                kWalkPresentationAction) {
            if (presentation_action_ !=
                action.requested_presentation_action) {
                action_counter_ = 0;
            }
            presentation_action_ =
                action.requested_presentation_action;
        }
        if (action.movement.mode !=
            MovementDestinationMode::none) {
            movement_destination_.initialize(
                action.movement, position_);
            movement_speed_ = action.movement.speed;
            movement_controller_.reset();
        }
    }

    if (run_presentation) {
        EnemyPresentationContext presentation_context;
        presentation_context.position = position_;
        presentation_context.direction = direction_;
        presentation_context.event_number =
            event_number_;
        presentation_context.resource_id =
            resource_id_;
        presentation_context.source_character_number =
            characterNumber();
        presentation_context.source_judgement =
            judgement_;
        presentation_context.profile =
            &presentation_profile_;
        presentation_context.animation =
            visual_ ? &visual_->animation() : nullptr;
        presentation_context.parameter_tables =
            context.parameter_tables;
        presentation_context.random = context.random;
        presentation_context.target_in_range =
            context.target_in_range;
        presentation_context.default_target =
            context.default_target;
        presentation_context.direct_impact_target =
            context.direct_impact_target;
        const EnemyPresentationUpdate presentation =
            presentation_.update(
                presentation_context);
        if (presentation.handled) {
            presentation_action_ =
                presentation.presentation_action;
            animation_chart_ =
                presentation.animation_chart;
            animation_frame_ =
                presentation.animation_frame;
            direction_ = presentation.direction;
        }
        for (std::int32_t sample :
             presentation.audio_samples) {
            if (sample >= 0) {
                result.audio_samples.push_back(sample);
            }
        }
        if (presentation.direct_impact.valid) {
            result.direct_impact =
                presentation.direct_impact;
            if (presentation
                    .direct_impact.post_hit_event != -1) {
                event_number_ =
                    presentation
                        .direct_impact.post_hit_event;
            }
        }
        if (presentation.effect_spawn.valid) {
            result.effect_spawn =
                presentation.effect_spawn;
        }
        if (presentation.completion_event != -1) {
            event_number_ =
                presentation.completion_event;
        }
        return result;
    }

    if (context.ground && context.objects &&
        context.random &&
        movement_destination_.request().mode !=
            MovementDestinationMode::none) {
        const MovementDestinationResult destination =
            movement_destination_.update(
                {
                    position_,
                    judgement_,
                    context.resolve_movement_target,
                },
                *context.random);
        if (destination.active) {
            const MovementStepResult movement =
                movement_controller_.advance(
                    *context.ground,
                    *context.objects,
                    judgement_,
                    position_,
                    destination.destination,
                    movement_speed_,
                    context.dynamic_blockers,
                    movementBlockerId());
            if (movement.moved) {
                direction_ = retailDirectionForVector(
                    movement.position.x - position_.x,
                    movement.position.y - position_.y);
            }
            position_ = movement.position;
            if (presentation_action_ !=
                kWalkPresentationAction) {
                presentation_action_ =
                    kWalkPresentationAction;
                movement_action_counter_ = 0;
            }
            animation_chart_ = kWalkAnimationChart;
            animation_frame_ =
                movement_action_counter_++;
            draw_strength_ = 1000;
            return result;
        }
        movement_destination_.reset();
        movement_controller_.reset();
        movement_speed_ = 0;
        presentation_action_ =
            kIdlePresentationAction;
        action_counter_ = 0;
    }

    // Enemy action seven is the retail idle action. It selects CAF chart
    // zero, submits the current counter, then advances it once per
    // active-map gameplay update.
    presentation_action_ = kIdlePresentationAction;
    animation_chart_ = 0;
    animation_frame_ = action_counter_++;
    draw_strength_ = 1000;
    return result;
}

EnemyActorUpdate EnemyActor::update(
    const GroundMap& ground,
    const ObjectMap& objects,
    const std::vector<MovementBlocker>* dynamic_blockers) {
    EnemyActorUpdateContext context;
    context.ground = &ground;
    context.objects = &objects;
    context.dynamic_blockers = dynamic_blockers;
    return update(context);
}

std::int32_t EnemyActor::stateValue(
    ScenarioEntityStateChannel channel) const {
    return state_.value(channel);
}

void EnemyActor::setStateValue(
    ScenarioEntityStateChannel channel,
    std::int32_t value) {
    state_.setValue(channel, value);
}

std::int32_t EnemyActor::id() const {
    return id_;
}

std::int32_t EnemyActor::characterNumber() const {
    return 14000000 + id_;
}

std::int32_t EnemyActor::movementBlockerId() const {
    return characterNumber();
}

std::int32_t EnemyActor::resourceId() const {
    return resource_id_;
}

const std::string& EnemyActor::name() const {
    return name_;
}

std::uint32_t EnemyActor::nameColor() const {
    return name_color_;
}

std::int32_t EnemyActor::labelHeight() const {
    return label_height_;
}

WorldPosition EnemyActor::position() const {
    return position_;
}

WorldPosition EnemyActor::renderPosition(double alpha) const {
    return interpolateWorldPosition(
        previous_position_, position_, alpha);
}

const ObjectBounds& EnemyActor::judgement() const {
    return judgement_;
}

std::int32_t EnemyActor::direction() const {
    return direction_;
}

std::int32_t EnemyActor::animationChart() const {
    return animation_chart_;
}

std::int32_t EnemyActor::animationFrame() const {
    return animation_frame_;
}

std::int32_t EnemyActor::drawStrength() const {
    return draw_strength_;
}

bool EnemyActor::expired() const {
    return expired_;
}

const std::string& EnemyActor::aiControlName() const {
    return ai_control_name_;
}

const AiControlList* EnemyActor::aiControl() const {
    return ai_control_;
}

std::int32_t EnemyActor::aiControlIndex() const {
    return ai_control_index_;
}

const ObjectBounds& EnemyActor::patrolBounds() const {
    return patrol_bounds_;
}

std::int32_t EnemyActor::currentLife() const {
    return current_life_;
}

std::int32_t EnemyActor::maximumLife() const {
    return maximum_life_;
}

std::int32_t EnemyActor::nativeElement() const {
    return native_element_;
}

std::int32_t EnemyActor::physicalDefense() const {
    return physical_defense_;
}

std::int32_t EnemyActor::physicalEvasion() const {
    return physical_evasion_;
}

std::int32_t EnemyActor::magicalDefense() const {
    return magical_defense_;
}

std::int32_t EnemyActor::magicalEvasion() const {
    return magical_evasion_;
}

std::int32_t EnemyActor::experienceReward() const {
    return experience_reward_;
}

std::int32_t EnemyActor::lootTableRow() const {
    return loot_table_row_;
}

std::int32_t EnemyActor::goldDropChance() const {
    return gold_drop_chance_;
}

std::int32_t EnemyActor::goldMinimum() const {
    return gold_minimum_;
}

std::int32_t EnemyActor::goldMaximum() const {
    return gold_maximum_;
}

std::int32_t EnemyActor::reactionChanceDefense() const {
    return reaction_chance_defense_;
}

std::int32_t EnemyActor::reactionDurationDefense() const {
    return reaction_duration_defense_;
}

bool EnemyActor::alwaysSuppressReactionDisplacement() const {
    return always_suppress_reaction_displacement_;
}

std::int32_t EnemyActor::movementSpeedScale() const {
    return movement_speed_scale_;
}

const EnemyPresentationProfile&
EnemyActor::presentationProfile() const {
    return presentation_profile_;
}

bool EnemyActor::partEnabled(std::size_t part) const {
    return part < part_visibility_.size() &&
           part_visibility_[part] != 0;
}

std::int32_t EnemyActor::partRedStrength(
    std::size_t part) const {
    return part < red_strength_.size()
        ? red_strength_[part]
        : 1000;
}

std::int32_t EnemyActor::partGreenStrength(
    std::size_t part) const {
    return part < green_strength_.size()
        ? green_strength_[part]
        : 1000;
}

std::int32_t EnemyActor::partBlueStrength(
    std::size_t part) const {
    return part < blue_strength_.size()
        ? blue_strength_[part]
        : 1000;
}

const gapi::NjpImage& EnemyActor::patterns() const {
    return visual_->patterns();
}

const gapi::NjpImage&
EnemyActor::shadowPatterns() const {
    return visual_->shadowPatterns();
}

const gapi::CafAnimation& EnemyActor::animation() const {
    return visual_->animation();
}

bool EnemyActor::hasVisual() const {
    return visual_ != nullptr;
}

bool EnemyActor::visible() const {
    return state_.visible();
}

bool EnemyActor::pointerEnabled() const {
    return state_.pointerEnabled();
}

bool EnemyActor::judgementEnabled() const {
    return state_.judgementEnabled();
}

EnemyDamageReceiverState EnemyActor::damageReceiverState(
    std::int32_t scenario_number) const {
    EnemyDamageReceiverState state;
    state.character_number = characterNumber();
    state.scenario_number = scenario_number;
    state.position = position_;
    state.judgement = judgement_;
    state.has_visual = hasVisual();
    state.current_life = current_life_;
    state.maximum_life = maximum_life_;
    state.native_element = native_element_;
    state.physical_defense = physical_defense_;
    state.magical_defense = magical_defense_;
    state.reaction_chance_defense =
        reaction_chance_defense_;
    state.reaction_duration_defense =
        reaction_duration_defense_;
    state.always_suppress_reaction_displacement =
        always_suppress_reaction_displacement_;
    state.presentation_action =
        presentation_action_;
    state.presentation_counter =
        action_counter_;
    state.action_lock = action_lock_;
    state.reaction_duration =
        reaction_duration_;
    state.reaction_stage = reaction_stage_;
    state.reaction_displacement_suppressed =
        reaction_displacement_suppressed_;
    state.reaction_additive = reaction_additive_;
    state.reaction_angle = reaction_angle_;
    state.direction = direction_;
    state.event_number = event_number_;
    state.attributed_damage =
        attributed_damage_;
    state.death_counter = death_counter_;
    state.defeated_by_effect =
        defeated_by_effect_;
    state.defeat_source_character_number =
        defeat_source_character_number_;
    return state;
}

void EnemyActor::applyDamageReceiverState(
    const EnemyDamageReceiverState& state) {
    if (state.character_number != characterNumber()) {
        return;
    }
    current_life_ = state.current_life;
    presentation_action_ =
        state.presentation_action;
    action_counter_ =
        state.presentation_counter;
    action_lock_ = state.action_lock;
    reaction_duration_ =
        state.reaction_duration;
    reaction_stage_ = state.reaction_stage;
    reaction_displacement_suppressed_ =
        state.reaction_displacement_suppressed;
    reaction_additive_ =
        state.reaction_additive;
    reaction_angle_ = state.reaction_angle;
    direction_ = state.direction;
    event_number_ = state.event_number;
    attributed_damage_ =
        state.attributed_damage;
    death_counter_ = state.death_counter;
    defeated_by_effect_ =
        state.defeated_by_effect;
    defeat_source_character_number_ =
        state.defeat_source_character_number;
    if (presentation_action_ ==
            kHitPresentationAction ||
        presentation_action_ ==
            kDeathPresentationAction) {
        presentation_.reset();
        movement_destination_.reset();
        movement_controller_.reset();
        movement_speed_ = 0;
        movement_action_counter_ = 0;
    }
}

bool enemyBlocksMovement(const EnemyActor& enemy) {
    return
        enemy.judgementEnabled() &&
        enemy.currentLife() > 0;
}

}  // namespace osf
