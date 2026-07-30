#ifndef OPENSHADOWFLARE_ITEM_GRID_HPP
#define OPENSHADOWFLARE_ITEM_GRID_HPP

#include "player_inventory.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace osf {

struct ItemGridOverlap {
    std::optional<std::size_t> item_index;
    bool multiple = false;
};

bool itemFootprintsOverlap(
    const InventoryItem& first,
    const InventoryItem& second);
bool itemContainsGridCell(
    const InventoryItem& item,
    std::int32_t grid_x,
    std::int32_t grid_y);
bool itemFitsGrid(
    const InventoryItem& item,
    std::int32_t grid_width,
    std::int32_t grid_height);
ItemGridOverlap findItemGridOverlap(
    const std::vector<InventoryItem>& items,
    const InventoryItem& item);

}  // namespace osf

#endif
