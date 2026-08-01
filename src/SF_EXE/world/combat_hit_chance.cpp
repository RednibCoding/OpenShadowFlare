#include "combat_hit_chance.hpp"

#include <algorithm>

namespace osf {

std::int32_t retailCombatHitChance(
    std::int32_t attack_value,
    std::int32_t defense_value) {
    return std::clamp(
        attack_value - defense_value,
        std::int32_t{20},
        std::int32_t{98});
}

}  // namespace osf
