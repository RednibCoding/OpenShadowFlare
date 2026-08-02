#ifndef OPENSHADOWFLARE_SCENARIO_NUMERIC_LABEL_COMMAND_HPP
#define OPENSHADOWFLARE_SCENARIO_NUMERIC_LABEL_COMMAND_HPP

#include "world/scenario_text_label.hpp"

#include <cstdint>
#include <vector>

namespace osf {

bool makeScenarioNumericLabel(
    const std::vector<std::int32_t>& arguments,
    WorldPosition anchor,
    ScenarioTextLabel& label);

}  // namespace osf

#endif
