#include "scenario_object_actor.hpp"

#include "resources/object_visual_resource.hpp"

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

bool ScenarioObjectActor::initialize(
    const ScenarioObject& object,
    const ObjectVisualResource* visual,
    std::string* error) {
    clear();
    if (!state_.initialize(object.initial_state_values)) {
        setError(
            error,
            "The object does not contain its three retail state values.");
        return false;
    }
    if (object.resource_id >= 0 && !visual) {
        setError(error, "The object visual resource is missing.");
        clear();
        return false;
    }

    id_ = object.id;
    resource_id_ = object.resource_id;
    name_ = object.name;
    name_color_ = object.name_color;
    label_height_ = object.label_height;
    position_ = {object.world_x, object.world_y};
    judgement_ = {
        object.judgement_left,
        object.judgement_top,
        object.judgement_right,
        object.judgement_bottom,
    };
    direction_ = object.direction;
    visual_mode_ = object.visual_mode;
    static_pattern_ = object.static_pattern;
    animation_chart_ = object.animation_chart;
    display_status_ =
        object.draw_flags |
        (object.draw_status_bit_80 ? 0x80 : 0);
    height_ = object.height;
    draw_flags_ = object.draw_flags;
    draw_strength_ = object.draw_strength;
    red_draw_strength_ = object.red_draw_strength;
    green_draw_strength_ = object.green_draw_strength;
    blue_draw_strength_ = object.blue_draw_strength;
    visual_ = visual;

    const std::size_t part_count =
        hasAnimatedVisual()
            ? visual_->animation().maxPartCount()
            : 0;
    part_visibility_.assign(part_count, 1);
    red_strength_.assign(part_count, 1000);
    green_strength_.assign(part_count, 1000);
    blue_strength_.assign(part_count, 1000);
    if (!object.part_visibility.empty()) {
        copyParts(part_visibility_, object.part_visibility);
        copyParts(red_strength_, object.red_strength);
        copyParts(green_strength_, object.green_strength);
        copyParts(blue_strength_, object.blue_strength);
    }

    if (visual_mode_ == 0 && animation_chart_ >= 0 &&
        (!visual_ || !visual_->hasAnimation())) {
        setError(
            error,
            "The object selects an animation without an animation resource.");
        clear();
        return false;
    }
    if (visual_mode_ != 0 && static_pattern_ >= 0 &&
        (!visual_ || !visual_->hasStaticPatterns())) {
        setError(
            error,
            "The object selects a static pattern without a pattern resource.");
        clear();
        return false;
    }
    if (error) {
        error->clear();
    }
    return true;
}

void ScenarioObjectActor::clear() {
    id_ = -1;
    resource_id_ = -1;
    name_.clear();
    name_color_ = 0;
    label_height_ = 0;
    position_ = {};
    judgement_ = {};
    direction_ = 0;
    visual_mode_ = 0;
    static_pattern_ = -1;
    animation_chart_ = -1;
    animation_frame_ = 0;
    display_status_ = 0;
    height_ = 0;
    draw_flags_ = 0;
    draw_strength_ = 0;
    red_draw_strength_ = 1000;
    green_draw_strength_ = 1000;
    blue_draw_strength_ = 1000;
    state_.clear();
    part_visibility_.clear();
    red_strength_.clear();
    green_strength_.clear();
    blue_strength_.clear();
    visual_ = nullptr;
}

void ScenarioObjectActor::update() {
    if (!visible() || !drawEnabled() ||
        !hasAnimatedVisual()) {
        return;
    }
    const auto& charts = animation().charts();
    if (animation_chart_ < 0 ||
        static_cast<std::size_t>(animation_chart_) >=
            charts.size()) {
        return;
    }
    const gapi::CafChart& chart =
        charts[static_cast<std::size_t>(animation_chart_)];
    if (direction_ < 0 ||
        static_cast<std::size_t>(direction_) >=
            chart.directions.size()) {
        return;
    }
    const std::int32_t frame_count =
        chart.directions[
            static_cast<std::size_t>(direction_)].frame_count;
    if (frame_count <= 0) {
        return;
    }
    if ((chart.status & 1) != 0) {
        animation_frame_ =
            (animation_frame_ + 1) % frame_count;
    } else {
        animation_frame_ =
            std::min(animation_frame_ + 1, frame_count - 1);
    }
}

std::int32_t ScenarioObjectActor::stateValue(
    ScenarioEntityStateChannel channel) const {
    return state_.value(channel);
}

void ScenarioObjectActor::setStateValue(
    ScenarioEntityStateChannel channel,
    std::int32_t value) {
    state_.setValue(channel, value);
}

void ScenarioObjectActor::setStateOverride(
    std::int32_t visible,
    std::int32_t pointer,
    std::int32_t judgement) {
    state_.setOverride(visible, pointer, judgement);
}

void ScenarioObjectActor::setDrawStrength(std::int32_t strength) {
    draw_strength_ = strength;
}

bool ScenarioObjectActor::stateOverrideEnabled() const {
    return state_.overrideEnabled();
}

std::int32_t ScenarioObjectActor::id() const {
    return id_;
}

std::int32_t ScenarioObjectActor::characterNumber() const {
    return 10000000 + id_;
}

std::int32_t ScenarioObjectActor::movementBlockerId() const {
    return characterNumber();
}

std::int32_t ScenarioObjectActor::resourceId() const {
    return resource_id_;
}

const std::string& ScenarioObjectActor::name() const {
    return name_;
}

std::uint32_t ScenarioObjectActor::nameColor() const {
    return name_color_;
}

std::int32_t ScenarioObjectActor::labelHeight() const {
    return label_height_;
}

WorldPosition ScenarioObjectActor::position() const {
    return position_;
}

const ObjectBounds& ScenarioObjectActor::judgement() const {
    return judgement_;
}

std::int32_t ScenarioObjectActor::direction() const {
    return direction_;
}

std::int32_t ScenarioObjectActor::staticPattern() const {
    return static_pattern_;
}

std::int32_t ScenarioObjectActor::animationChart() const {
    return animation_chart_;
}

std::int32_t ScenarioObjectActor::animationFrame() const {
    return animation_frame_;
}

std::int32_t ScenarioObjectActor::displayStatus() const {
    return display_status_;
}

std::int32_t ScenarioObjectActor::displayHeight() const {
    return height_ * 20 / 100;
}

std::int32_t ScenarioObjectActor::drawStrength() const {
    return draw_strength_;
}

std::int32_t ScenarioObjectActor::redDrawStrength() const {
    return red_draw_strength_;
}

std::int32_t ScenarioObjectActor::greenDrawStrength() const {
    return green_draw_strength_;
}

std::int32_t ScenarioObjectActor::blueDrawStrength() const {
    return blue_draw_strength_;
}

bool ScenarioObjectActor::partEnabled(std::size_t part) const {
    return part < part_visibility_.size() &&
           part_visibility_[part] != 0;
}

std::int32_t ScenarioObjectActor::partRedStrength(
    std::size_t part) const {
    return part < red_strength_.size()
               ? red_strength_[part] * red_draw_strength_ / 1000
               : red_draw_strength_;
}

std::int32_t ScenarioObjectActor::partGreenStrength(
    std::size_t part) const {
    return part < green_strength_.size()
               ? green_strength_[part] * green_draw_strength_ / 1000
               : green_draw_strength_;
}

std::int32_t ScenarioObjectActor::partBlueStrength(
    std::size_t part) const {
    return part < blue_strength_.size()
               ? blue_strength_[part] * blue_draw_strength_ / 1000
               : blue_draw_strength_;
}

bool ScenarioObjectActor::hasStaticVisual() const {
    return visual_mode_ != 0 &&
           static_pattern_ >= 0 &&
           visual_ &&
           visual_->hasStaticPatterns();
}

bool ScenarioObjectActor::hasStaticShadow() const {
    return hasStaticVisual() && visual_->hasStaticShadows();
}

bool ScenarioObjectActor::hasAnimatedVisual() const {
    return visual_mode_ == 0 &&
           animation_chart_ >= 0 &&
           visual_ &&
           visual_->hasAnimation();
}

bool ScenarioObjectActor::drawEnabled() const {
    return visible() &&
           ((draw_flags_ & 4) != 0 ||
            draw_strength_ != 0) &&
           (hasStaticVisual() || hasAnimatedVisual());
}

const gapi::NjpImage&
ScenarioObjectActor::staticPatterns() const {
    return visual_->staticPatterns();
}

const gapi::NjpImage&
ScenarioObjectActor::staticShadows() const {
    return visual_->staticShadows();
}

const gapi::NjpImage&
ScenarioObjectActor::animationPatterns() const {
    return visual_->animationPatterns();
}

const gapi::CafAnimation&
ScenarioObjectActor::animation() const {
    return visual_->animation();
}

bool ScenarioObjectActor::visible() const {
    return state_.visible();
}

bool ScenarioObjectActor::pointerEnabled() const {
    return state_.pointerEnabled();
}

bool ScenarioObjectActor::judgementEnabled() const {
    return state_.judgementEnabled();
}

}  // namespace osf
