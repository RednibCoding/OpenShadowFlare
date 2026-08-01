#ifndef OPENSHADOWFLARE_SCENARIO_EFFECT_COMMAND_HPP
#define OPENSHADOWFLARE_SCENARIO_EFFECT_COMMAND_HPP

#include "world/combat_effect_request.hpp"

#include <cstdint>
#include <vector>

namespace osf {

bool makeScenarioEffectRequest(
    const std::vector<std::int32_t>& arguments,
    std::int32_t random_value,
    CombatEffectSpawnRequest& request);

}  // namespace osf

#endif
