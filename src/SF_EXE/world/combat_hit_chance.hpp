#ifndef OPENSHADOWFLARE_COMBAT_HIT_CHANCE_HPP
#define OPENSHADOWFLARE_COMBAT_HIT_CHANCE_HPP

#include <cstdint>

namespace osf {

std::int32_t retailCombatHitChance(
    std::int32_t attack_value,
    std::int32_t defense_value);

}  // namespace osf

#endif
