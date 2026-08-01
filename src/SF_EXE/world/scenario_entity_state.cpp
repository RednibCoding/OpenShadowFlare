#include "scenario_entity_state.hpp"

#include <algorithm>

namespace osf {

bool ScenarioEntityState::initialize(
    const std::vector<std::int32_t>& initial_values) {
    clear();
    if (initial_values.size() != values_.size()) {
        return false;
    }
    std::copy_n(
        initial_values.begin(),
        values_.size(),
        values_.begin());
    return true;
}

void ScenarioEntityState::clear() {
    values_.fill(0);
    override_values_.fill(0);
    override_enabled_ = false;
}

std::int32_t ScenarioEntityState::value(
    ScenarioEntityStateChannel channel) const {
    return values_[static_cast<std::size_t>(channel)];
}

void ScenarioEntityState::setValue(
    ScenarioEntityStateChannel channel,
    std::int32_t value) {
    values_[static_cast<std::size_t>(channel)] = value;
}

void ScenarioEntityState::setOverride(
    std::int32_t visible,
    std::int32_t pointer,
    std::int32_t judgement) {
    override_values_ = {visible, pointer, judgement};
    override_enabled_ = true;
}

bool ScenarioEntityState::overrideEnabled() const {
    return override_enabled_;
}

std::int32_t ScenarioEntityState::effectiveValue(
    ScenarioEntityStateChannel channel) const {
    const std::size_t index = static_cast<std::size_t>(channel);
    return override_enabled_ ? override_values_[index] : values_[index];
}

bool ScenarioEntityState::visible() const {
    return effectiveValue(
               ScenarioEntityStateChannel::visible) != 0;
}

bool ScenarioEntityState::pointerEnabled() const {
    return effectiveValue(
               ScenarioEntityStateChannel::pointer) != 0;
}

bool ScenarioEntityState::judgementEnabled() const {
    return effectiveValue(
               ScenarioEntityStateChannel::judgement) != 0;
}

}  // namespace osf
