#include "vendor_inventory.hpp"

#include "item_grid.hpp"

#include <utility>

namespace osf {

void VendorInventory::clear() {
    items_.clear();
}

bool VendorInventory::store(InventoryItem item) {
    return store(std::move(item), 0, 0);
}

bool VendorInventory::store(
    InventoryItem item,
    std::int32_t start_x,
    std::int32_t start_y) {
    if (start_x < 0 || start_x >= grid_width ||
        start_y < 0 || start_y >= grid_height) {
        start_x = 0;
        start_y = 0;
    }
    const std::int32_t first = start_y * grid_width + start_x;
    for (std::int32_t cell = first;
         cell < grid_width * grid_height;
         ++cell) {
        item.grid_x = cell % grid_width;
        item.grid_y = cell / grid_width;
        if (itemFitsGrid(item, grid_width, grid_height) &&
            !findItemGridOverlap(items_, item).item_index) {
            items_.push_back(std::move(item));
            return true;
        }
    }
    return false;
}

bool VendorInventory::place(InventoryItem item) {
    if (!itemFitsGrid(item, grid_width, grid_height) ||
        findItemGridOverlap(items_, item).item_index) {
        return false;
    }
    items_.push_back(std::move(item));
    return true;
}

std::optional<InventoryItem> VendorInventory::take(
    std::size_t item_index) {
    if (item_index >= items_.size()) {
        return std::nullopt;
    }
    InventoryItem item = std::move(items_[item_index]);
    items_.erase(items_.begin() +
                 static_cast<std::ptrdiff_t>(item_index));
    return item;
}

const InventoryItem* VendorInventory::itemAt(
    std::int32_t grid_x,
    std::int32_t grid_y) const {
    for (const InventoryItem& item : items_) {
        if (itemContainsGridCell(item, grid_x, grid_y)) {
            return &item;
        }
    }
    return nullptr;
}

const std::vector<InventoryItem>& VendorInventory::items() const {
    return items_;
}

}  // namespace osf
