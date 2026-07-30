#include "player_inventory.hpp"

#include "item_database.hpp"
#include "item_grid.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <utility>

namespace osf {
namespace {

constexpr std::int32_t kGoldCategory = 4;
constexpr std::int32_t kGoldDefinition = 0;

std::int32_t initiallyIdentified(
    const ItemDefinition& definition) {
    return definition.variant != 1 &&
                   definition.variant != 2
        ? 1
        : 0;
}

bool findPlacement(
    const std::vector<InventoryItem>& items,
    std::int32_t width,
    std::int32_t height,
    std::int32_t& grid_x,
    std::int32_t& grid_y) {
    if (width <= 0 || height <= 0 ||
        width > PlayerInventory::grid_width ||
        height > PlayerInventory::grid_height) {
        return false;
    }

    std::array<
        bool,
        static_cast<std::size_t>(
            PlayerInventory::grid_width *
            PlayerInventory::grid_height)> occupied{};
    for (const InventoryItem& item : items) {
        for (std::int32_t y = 0; y < item.height; ++y) {
            for (std::int32_t x = 0; x < item.width; ++x) {
                const std::int32_t occupied_x = item.grid_x + x;
                const std::int32_t occupied_y = item.grid_y + y;
                if (occupied_x >= 0 &&
                    occupied_x < PlayerInventory::grid_width &&
                    occupied_y >= 0 &&
                    occupied_y < PlayerInventory::grid_height) {
                    occupied[
                        static_cast<std::size_t>(
                            occupied_y *
                                PlayerInventory::grid_width +
                            occupied_x)] = true;
                }
            }
        }
    }

    for (std::int32_t y = 0;
         y <= PlayerInventory::grid_height - height;
         ++y) {
        for (std::int32_t x = 0;
             x <= PlayerInventory::grid_width - width;
             ++x) {
            bool available = true;
            for (std::int32_t item_y = 0;
                 item_y < height && available;
                 ++item_y) {
                for (std::int32_t item_x = 0;
                     item_x < width;
                     ++item_x) {
                    if (occupied[
                            static_cast<std::size_t>(
                                (y + item_y) *
                                    PlayerInventory::grid_width +
                                x + item_x)]) {
                        available = false;
                        break;
                    }
                }
            }
            if (available) {
                grid_x = x;
                grid_y = y;
                return true;
            }
        }
    }
    return false;
}

}  // namespace

InventoryItem makeInventoryItem(
    const ItemDefinition& definition,
    std::int32_t quantity) {
    InventoryItem item;
    item.category = definition.category;
    item.definition_id = definition.id;
    item.quantity = quantity;
    item.width = definition.inventory_width;
    item.height = definition.inventory_height;
    item.durability = definition.maximum_durability;
    item.identified = initiallyIdentified(definition);
    return item;
}

void PlayerInventory::clear() {
    items_.clear();
}

bool PlayerInventory::add(
    std::int32_t category,
    std::int32_t definition_id,
    std::int32_t quantity) {
    return add(
        category,
        definition_id,
        quantity,
        1,
        1,
        -1,
        0);
}

bool PlayerInventory::add(
    const ItemDefinition& definition,
    std::int32_t quantity) {
    return add(
        definition.category,
        definition.id,
        quantity,
        definition.inventory_width,
        definition.inventory_height,
        definition.maximum_durability,
        initiallyIdentified(definition));
}

bool PlayerInventory::add(
    std::int32_t category,
    std::int32_t definition_id,
    std::int32_t quantity,
    std::int32_t width,
    std::int32_t height,
    std::int32_t durability,
    std::int32_t identified) {
    if (quantity <= 0) {
        return false;
    }

    std::vector<InventoryItem> updated = items_;
    std::int32_t remaining = quantity;
    if (category == kGoldCategory &&
        definition_id == kGoldDefinition) {
        for (InventoryItem& owned : updated) {
            if (owned.category != category ||
                owned.definition_id != definition_id ||
                owned.quantity >= maximum_gold_stack) {
                continue;
            }
            const std::int32_t moved = std::min(
                remaining,
                maximum_gold_stack - owned.quantity);
            owned.quantity += moved;
            remaining -= moved;
            if (remaining == 0) {
                items_ = std::move(updated);
                return true;
            }
        }
    }

    const std::int32_t stack_limit =
        category == kGoldCategory &&
                definition_id == kGoldDefinition
            ? maximum_gold_stack
            : 1;
    while (remaining > 0) {
        std::int32_t grid_x = 0;
        std::int32_t grid_y = 0;
        if (!findPlacement(
                updated,
                width,
                height,
                grid_x,
                grid_y)) {
            return false;
        }
        const std::int32_t stack_quantity =
            std::min(remaining, stack_limit);
        updated.push_back({
            category,
            definition_id,
            stack_quantity,
            grid_x,
            grid_y,
            width,
            height,
            durability,
            identified,
            {},
        });
        remaining -= stack_quantity;
    }
    items_ = std::move(updated);
    return true;
}

std::optional<InventoryItem> PlayerInventory::take(
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

InventoryPlacementResult PlayerInventory::place(
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
            maximum_gold_stack -
            overlapping.quantity);
        if (moved <= 0) {
            return {};
        }
        overlapping.quantity += moved;
        item.quantity -= moved;
        if (item.quantity == 0) {
            return {true, std::nullopt};
        }
        return {true, std::move(item)};
    }

    // Retail leaves the displaced item on the pointer. Its old grid
    // coordinates remain attached to the item until it is placed again.
    InventoryItem displaced = overlapping;
    items_.erase(
        items_.begin() +
        static_cast<std::ptrdiff_t>(
            *overlap.item_index));
    items_.push_back(std::move(item));
    return {true, std::move(displaced)};
}

const InventoryItem* PlayerInventory::itemAt(
    std::int32_t grid_x,
    std::int32_t grid_y) const {
    for (const InventoryItem& item : items_) {
        if (itemContainsGridCell(
                item, grid_x, grid_y)) {
            return &item;
        }
    }
    return nullptr;
}

const std::vector<InventoryItem>&
PlayerInventory::items() const {
    return items_;
}

std::int32_t PlayerInventory::gold() const {
    std::int64_t total = 0;
    for (const InventoryItem& item : items_) {
        if (item.category == kGoldCategory &&
            item.definition_id == kGoldDefinition) {
            total += item.quantity;
        }
    }
    return static_cast<std::int32_t>(
        std::min<std::int64_t>(
            total,
            std::numeric_limits<std::int32_t>::max()));
}

}  // namespace osf
