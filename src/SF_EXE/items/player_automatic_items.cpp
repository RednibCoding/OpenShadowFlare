#include "player_automatic_items.hpp"

#include "item_database.hpp"
#include "item_grid.hpp"

#include <cstddef>
#include <utility>

namespace osf {

void PlayerAutomaticItems::clear() {
    for (PlayerSpecialItems& storage : pages_) {
        storage.clear();
    }
}

bool PlayerAutomaticItems::add(
    const ItemDefinition& definition,
    InventoryItem item) {
    if (definition.automatic_inventory_page < 0 ||
        definition.automatic_inventory_page >=
            static_cast<std::int32_t>(page_count)) {
        return false;
    }
    PlayerSpecialItems& storage = pages_[
        static_cast<std::size_t>(
            definition.automatic_inventory_page)];
    if (storage.contains(
            item.category, item.definition_id)) {
        return false;
    }
    item.grid_x = definition.automatic_inventory_x;
    item.grid_y = definition.automatic_inventory_y;
    const ItemGridOverlap overlap =
        findItemGridOverlap(storage.items(), item);
    if (!itemFitsGrid(
            item,
            PlayerSpecialItems::grid_width,
            PlayerSpecialItems::grid_height) ||
        overlap.item_index || overlap.multiple) {
        return false;
    }
    const InventoryPlacementResult placed = storage.place(
        std::move(item),
        definition.automatic_inventory_x,
        definition.automatic_inventory_y);
    return placed.accepted && !placed.held_item;
}

bool PlayerAutomaticItems::contains(
    std::int32_t category,
    std::int32_t definition_id) const {
    for (const PlayerSpecialItems& storage : pages_) {
        if (storage.contains(category, definition_id)) {
            return true;
        }
    }
    return false;
}

bool PlayerAutomaticItems::removeFirst(
    std::int32_t category,
    std::int32_t definition_id) {
    for (PlayerSpecialItems& storage : pages_) {
        const auto& items = storage.items();
        for (std::size_t index = 0; index < items.size(); ++index) {
            if (items[index].category == category &&
                items[index].definition_id == definition_id) {
                return storage.take(index).has_value();
            }
        }
    }
    return false;
}

PlayerSpecialItems& PlayerAutomaticItems::page(
    std::size_t page_index) {
    return pages_.at(page_index);
}

const PlayerSpecialItems& PlayerAutomaticItems::page(
    std::size_t page_index) const {
    return pages_.at(page_index);
}

}  // namespace osf
