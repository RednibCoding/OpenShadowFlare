#ifndef OPENSHADOWFLARE_ITEM_REPAIR_HPP
#define OPENSHADOWFLARE_ITEM_REPAIR_HPP

#include <cstdint>

namespace osf {

struct InventoryItem;
struct ItemDefinition;
class TableData;

std::int32_t retailItemValue(
    const InventoryItem& item,
    const ItemDefinition& definition,
    const TableData& value_parameters);

std::int32_t retailItemRepairPrice(
    const InventoryItem& item,
    const ItemDefinition& definition,
    const TableData& value_parameters);

bool repairInventoryItem(
    InventoryItem& item,
    const ItemDefinition& definition);

}  // namespace osf

#endif
