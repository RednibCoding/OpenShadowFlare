#include "item_grid.hpp"

namespace osf {

bool itemFootprintsOverlap(
    const InventoryItem& first,
    const InventoryItem& second) {
    return first.grid_x < second.grid_x + second.width &&
           first.grid_x + first.width > second.grid_x &&
           first.grid_y < second.grid_y + second.height &&
           first.grid_y + first.height > second.grid_y;
}

bool itemContainsGridCell(
    const InventoryItem& item,
    std::int32_t grid_x,
    std::int32_t grid_y) {
    return grid_x >= item.grid_x &&
           grid_x < item.grid_x + item.width &&
           grid_y >= item.grid_y &&
           grid_y < item.grid_y + item.height;
}

bool itemFitsGrid(
    const InventoryItem& item,
    std::int32_t grid_width,
    std::int32_t grid_height) {
    return item.width > 0 &&
           item.height > 0 &&
           item.grid_x >= 0 &&
           item.grid_y >= 0 &&
           item.grid_x + item.width <= grid_width &&
           item.grid_y + item.height <= grid_height;
}

ItemGridOverlap findItemGridOverlap(
    const std::vector<InventoryItem>& items,
    const InventoryItem& item) {
    ItemGridOverlap result;
    for (std::size_t index = 0; index < items.size(); ++index) {
        if (!itemFootprintsOverlap(item, items[index])) {
            continue;
        }
        if (result.item_index) {
            result.multiple = true;
            return result;
        }
        result.item_index = index;
    }
    return result;
}

}  // namespace osf
