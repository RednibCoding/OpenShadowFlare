#ifndef OPENSHADOWFLARE_GROUND_ITEM_HPP
#define OPENSHADOWFLARE_GROUND_ITEM_HPP

#include "core/retail_random.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"

#include <cstdint>
#include <vector>

namespace osf {

struct GroundItem {
    std::int32_t category = 0;
    std::int32_t definition_id = 0;
    std::int32_t quantity = 1;
    WorldPosition position;
};

bool createGroundItems(
    std::vector<GroundItem>& items,
    RetailRandom& random,
    std::int32_t category,
    std::int32_t definition_id,
    WorldPosition position,
    std::int32_t minimum_quantity,
    std::int32_t maximum_quantity);

}  // namespace osf

#endif
