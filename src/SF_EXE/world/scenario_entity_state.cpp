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

bool ScenarioEntityState::visible() const {
    return value(ScenarioEntityStateChannel::visible) != 0;
}

bool ScenarioEntityState::pointerEnabled() const {
    return value(ScenarioEntityStateChannel::pointer) != 0;
}

bool ScenarioEntityState::judgementEnabled() const {
    return value(ScenarioEntityStateChannel::judgement) != 0;
}

}  // namespace osf
