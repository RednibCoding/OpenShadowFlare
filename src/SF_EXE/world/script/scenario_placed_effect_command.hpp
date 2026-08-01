#ifndef OPENSHADOWFLARE_SCENARIO_PLACED_EFFECT_COMMAND_HPP
#define OPENSHADOWFLARE_SCENARIO_PLACED_EFFECT_COMMAND_HPP

#include "world/combat_effect_request.hpp"

#include <cstdint>
#include <vector>

namespace osf {

bool makeScenarioPlacedEffectRequest(
    const std::vector<std::int32_t>& arguments,
    CombatEffectSpawnRequest& request);

}  // namespace osf

#endif
