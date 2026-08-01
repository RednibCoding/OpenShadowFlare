#include "player_belt.hpp"

#include "item_database.hpp"
#include "item_grid.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace osf {
void PlayerBelt::clear() {
    items_.clear();
}

InventoryPlacementResult PlayerBelt::place(
    InventoryItem item,
    std::int32_t grid_x,
    std::int32_t grid_y,
    const ItemDefinition& definition) {
    // The HUD pocket branch in FUN_00445bd0 accepts only category-three
    // items and places their complete inventory footprint in a 4-by-2 grid.
    if (item.category != 3 ||
        definition.category != item.category ||
        definition.id != item.definition_id ||
        item.quantity != 1 ||
        item.width <= 0 ||
        item.height <= 0 ||
        item.width != definition.inventory_width ||
        item.height != definition.inventory_height) {
        return {};
    }

    item.grid_x = grid_x;
    item.grid_y = grid_y;
    if (!itemFitsGrid(item, grid_width, grid_height)) {
        return {};
    }

    const ItemGridOverlap overlap =
        findItemGridOverlap(items_, item);
    if (overlap.multiple) {
        return {};
    }
    if (!overlap.item_index) {
        items_.push_back(std::move(item));
        return {true, std::nullopt};
    }

    InventoryItem displaced = items_[*overlap.item_index];
    items_.erase(
        items_.begin() +
        static_cast<std::ptrdiff_t>(*overlap.item_index));
    items_.push_back(std::move(item));
    return {true, std::move(displaced)};
}

std::optional<InventoryItem> PlayerBelt::takeAt(
    std::int32_t grid_x,
    std::int32_t grid_y) {
    for (std::size_t index = 0; index < items_.size(); ++index) {
        if (!itemContainsGridCell(
                items_[index], grid_x, grid_y)) {
            continue;
        }
        InventoryItem item = items_[index];
        items_.erase(
            items_.begin() +
            static_cast<std::ptrdiff_t>(index));
        return item;
    }
    return std::nullopt;
}

const InventoryItem* PlayerBelt::itemAt(
    std::int32_t grid_x,
    std::int32_t grid_y) const {
    for (const InventoryItem& item : items_) {
        if (itemContainsGridCell(item, grid_x, grid_y)) {
            return &item;
        }
    }
    return nullptr;
}

const std::vector<InventoryItem>& PlayerBelt::items() const {
    return items_;
}

std::int32_t PlayerBelt::identifyAll() {
    std::int32_t identified = 0;
    for (InventoryItem& item : items_) {
        identified += identifyInventoryItem(item) ? 1 : 0;
    }
    return identified;
}

bool PlayerBelt::hasUnidentifiedItems() const {
    return std::any_of(
        items_.begin(),
        items_.end(),
        [](const InventoryItem& item) {
            return item.identified == 0;
        });
}

}  // namespace osf
