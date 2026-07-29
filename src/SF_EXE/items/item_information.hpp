#ifndef OPENSHADOWFLARE_ITEM_INFORMATION_HPP
#define OPENSHADOWFLARE_ITEM_INFORMATION_HPP

#include <cstdint>
#include <string>

namespace osf {

struct InventoryItem;
struct ItemDefinition;

std::int32_t itemSalePrice(
    const InventoryItem& item,
    const ItemDefinition& definition);
std::string itemInformationText(
    const InventoryItem& item,
    const ItemDefinition& definition);

}  // namespace osf

#endif
