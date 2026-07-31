#ifndef OPENSHADOWFLARE_COMBAT_EFFECT_NUMBER_HPP
#define OPENSHADOWFLARE_COMBAT_EFFECT_NUMBER_HPP

#include <cstdint>

namespace osf {

constexpr bool isDeathSplatterEffect(
    std::int32_t effect_number) {
    return effect_number >= 21000 &&
           effect_number <= 21003;
}

}  // namespace osf

#endif
