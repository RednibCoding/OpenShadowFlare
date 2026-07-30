#include "enemy_actor.hpp"

#include "libs/RKC_RPG_AICONTROL/rkc_rpg_aicontrol.hpp"
#include "resources/character_visual_resource.hpp"

#include <algorithm>
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
    movement_speed_scale_ =
        enemy.movement_speed_scale;
    presentation_profile_ = enemy.presentation;

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
    judgement_ = {};
    direction_ = 0;
    animation_frame_ = 0;
    action_counter_ = 0;
    ai_control_name_.clear();
    ai_control_ = nullptr;
    ai_control_index_ = -1;
    patrol_bounds_ = {};
    current_life_ = 0;
    maximum_life_ = 0;
    movement_speed_scale_ = 0;
    presentation_profile_ = {};
    state_.clear();
    part_visibility_.clear();
    red_strength_.clear();
    green_strength_.clear();
    blue_strength_.clear();
    visual_ = nullptr;
}

void EnemyActor::update() {
    // Enemy action seven is the retail idle action. It selects CAF chart
    // zero, submits the current counter, then advances it once per
    // active-map gameplay update.
    animation_frame_ = action_counter_++;
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

const ObjectBounds& EnemyActor::judgement() const {
    return judgement_;
}

std::int32_t EnemyActor::direction() const {
    return direction_;
}

std::int32_t EnemyActor::animationChart() const {
    return 0;
}

std::int32_t EnemyActor::animationFrame() const {
    return animation_frame_;
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

}  // namespace osf
