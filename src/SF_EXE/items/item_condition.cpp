#include "item_condition.hpp"

#include "item_database.hpp"
#include "player_inventory.hpp"

#include <algorithm>
#include <cstdint>

namespace osf {

std::int32_t itemCurrentDurability(
    const InventoryItem& item,
    const ItemDefinition& definition) {
    return item.durability >= 0
        ? item.durability
        : definition.maximum_durability;
}

bool itemConditionWarningVisible(
    const InventoryItem& item,
    const ItemDefinition& definition,
    std::uint32_t gameplay_counter) {
    if ((item.category != 0 && item.category != 1) ||
        item.category != definition.category ||
        item.definition_id != definition.id ||
        definition.maximum_durability <= 0) {
        return false;
    }

    const std::int32_t durability =
        std::clamp(
            itemCurrentDurability(item, definition),
            std::int32_t{0},
            definition.maximum_durability);
    if (durability == 0) {
        return true;
    }
    if ((static_cast<std::int64_t>(durability) * 100) /
            definition.maximum_durability >
        9) {
        return false;
    }
    return (gameplay_counter & 0x0fu) <= 7u;
}

}  // namespace osf
