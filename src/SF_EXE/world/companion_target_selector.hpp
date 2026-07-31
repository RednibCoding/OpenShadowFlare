#ifndef OPENSHADOWFLARE_COMPANION_TARGET_SELECTOR_HPP
#define OPENSHADOWFLARE_COMPANION_TARGET_SELECTOR_HPP

#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <cstdint>
#include <vector>

namespace osf {

struct CompanionEnemyTargetState {
    std::int32_t character_number = -1;
    WorldPosition position;
    ObjectBounds judgement;
    std::int32_t current_life = 0;
    std::int32_t physical_evasion = 0;
    bool active = true;
};

struct CompanionEnemyTarget {
    bool found = false;
    std::int32_t character_number = -1;
    WorldPosition position;
    ObjectBounds judgement;
    std::int32_t distance = 0;
    std::int32_t physical_evasion = 0;
};

CompanionEnemyTarget findCompanionEnemyTarget(
    WorldPosition position,
    const ObjectBounds& judgement,
    const std::vector<CompanionEnemyTargetState>& targets,
    std::int32_t maximum_distance);

CompanionEnemyTarget findCompanionForwardEnemyTarget(
    WorldPosition position,
    const ObjectBounds& judgement,
    std::int32_t direction,
    const std::vector<CompanionEnemyTargetState>& targets,
    std::int32_t maximum_distance);

}  // namespace osf

#endif
