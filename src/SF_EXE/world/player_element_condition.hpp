#ifndef OPENSHADOWFLARE_PLAYER_ELEMENT_CONDITION_HPP
#define OPENSHADOWFLARE_PLAYER_ELEMENT_CONDITION_HPP

#include <array>
#include <cstdint>

namespace osf {

struct ElementAnchor {
    std::int32_t x = 0;
    std::int32_t y = 0;
};

const std::array<ElementAnchor, 8>& retailElementAnchors();

ElementAnchor moveRetailElementCondition(
    ElementAnchor current,
    std::int32_t element,
    std::int32_t distance);

}  // namespace osf

#endif
