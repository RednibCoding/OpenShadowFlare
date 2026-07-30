#ifndef OPENSHADOWFLARE_ITEM_INSTANCE_FACTORY_HPP
#define OPENSHADOWFLARE_ITEM_INSTANCE_FACTORY_HPP

#include "player_inventory.hpp"

#include <cstdint>
#include <functional>

namespace osf {

struct ItemDefinition;

using ItemRandomSource = std::function<std::int32_t()>;

InventoryItem makeRetailInventoryItem(
    const ItemDefinition& definition,
    const ItemRandomSource& random,
    std::int32_t quantity = 1);

}  // namespace osf

#endif
