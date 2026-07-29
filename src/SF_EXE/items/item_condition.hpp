#ifndef OPENSHADOWFLARE_ITEM_CONDITION_HPP
#define OPENSHADOWFLARE_ITEM_CONDITION_HPP

#include <cstdint>

namespace osf {

struct InventoryItem;
struct ItemDefinition;

std::int32_t itemCurrentDurability(
    const InventoryItem& item,
    const ItemDefinition& definition);

bool itemConditionWarningVisible(
    const InventoryItem& item,
    const ItemDefinition& definition,
    std::uint32_t gameplay_counter);

}  // namespace osf

#endif
