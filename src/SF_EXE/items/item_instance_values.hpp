#ifndef OPENSHADOWFLARE_ITEM_INSTANCE_VALUES_HPP
#define OPENSHADOWFLARE_ITEM_INSTANCE_VALUES_HPP

#include <cstddef>
#include <cstdint>

namespace osf {

struct InventoryItem;
struct ItemDefinition;

std::int32_t retailItemRolledElementStrength(
    const InventoryItem& item,
    std::size_t element);

std::int32_t retailItemInstanceParameter(
    const InventoryItem& item,
    std::size_t parameter);

std::int32_t retailItemElementStrength(
    const InventoryItem& item,
    const ItemDefinition& definition,
    std::size_t element);

}  // namespace osf

#endif
