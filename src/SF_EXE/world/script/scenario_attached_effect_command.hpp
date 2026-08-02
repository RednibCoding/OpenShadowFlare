#ifndef OPENSHADOWFLARE_SCENARIO_ATTACHED_EFFECT_COMMAND_HPP
#define OPENSHADOWFLARE_SCENARIO_ATTACHED_EFFECT_COMMAND_HPP

#include "world/combat_effect_request.hpp"

#include <cstdint>
#include <vector>

namespace osf {

bool makeScenarioAttachedEffectRequest(
    const std::vector<std::int32_t>& arguments,
    std::int32_t owner_kind,
    const ObjectBounds& source_judgement,
    CombatEffectSpawnRequest& request);

}  // namespace osf

#endif
