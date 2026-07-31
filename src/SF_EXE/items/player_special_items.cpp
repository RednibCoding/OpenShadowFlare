#include "player_special_items.hpp"

#include "item_grid.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace osf {
namespace {

constexpr std::int32_t kGoldCategory = 4;
constexpr std::int32_t kGoldDefinition = 0;

}  // namespace

void PlayerSpecialItems::clear() {
    items_.clear();
}

InventoryPlacementResult PlayerSpecialItems::place(
    InventoryItem item,
    std::int32_t grid_x,
    std::int32_t grid_y) {
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

    InventoryItem& overlapping =
        items_[*overlap.item_index];
    if (item.category == kGoldCategory &&
        item.definition_id == kGoldDefinition &&
        overlapping.category == kGoldCategory &&
        overlapping.definition_id == kGoldDefinition) {
        const std::int32_t moved = std::min(
            item.quantity,
            PlayerInventory::maximum_gold_stack -
                overlapping.quantity);
        if (moved <= 0) {
            return {};
        }
        overlapping.quantity += moved;
        item.quantity -= moved;
        return {
            true,
            item.quantity == 0
                ? std::nullopt
                : std::optional<InventoryItem>(
                      std::move(item)),
        };
    }

    InventoryItem displaced = overlapping;
    items_.erase(
        items_.begin() +
        static_cast<std::ptrdiff_t>(
            *overlap.item_index));
    items_.push_back(std::move(item));
    return {true, std::move(displaced)};
}

std::optional<InventoryItem> PlayerSpecialItems::take(
    std::size_t item_index) {
    if (item_index >= items_.size()) {
        return std::nullopt;
    }
    InventoryItem item = items_[item_index];
    items_.erase(
        items_.begin() +
        static_cast<std::ptrdiff_t>(item_index));
    return item;
}

const std::vector<InventoryItem>&
PlayerSpecialItems::items() const {
    return items_;
}

bool PlayerSpecialItems::contains(
    std::int32_t category,
    std::int32_t definition_id) const {
    return std::any_of(
        items_.begin(),
        items_.end(),
        [category, definition_id](const InventoryItem& item) {
            return item.category == category &&
                   item.definition_id == definition_id;
        });
}

}  // namespace osf
