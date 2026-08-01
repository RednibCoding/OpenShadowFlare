#include "combat_hit_chance.hpp"

#include <algorithm>

namespace osf {

std::int32_t retailCombatHitChance(
    std::int32_t attack_value,
    std::int32_t defense_value) {
    return std::clamp(
        attack_value - defense_value,
        20,
        98);
}

}  // namespace osf
