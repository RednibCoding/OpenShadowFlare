#include "gameplay_vendor.hpp"

#include "gameplay_inventory.hpp"
#include "items/item_audio.hpp"
#include "items/item_database.hpp"
#include "items/item_information.hpp"
#include "items/player_inventory.hpp"
#include "items/vendor_inventory.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>

namespace osf {

void GameplayVendor::open(std::int32_t inventory_index) {
    active_ = inventory_index >= 0;
    inventory_index_ = active_ ? inventory_index : -1;
    hovered_item_index_ = -1;
    item_hover_updates_ = 0;
}

void GameplayVendor::close() {
    active_ = false;
    inventory_index_ = -1;
    hovered_item_index_ = -1;
    item_hover_updates_ = 0;
}

GameplayVendorResult GameplayVendor::update(
    const GameplayVendorInput& input,
    VendorInventory& vendor,
    GameplayInventory& held,
    PlayerInventory& player,
    const ItemDatabase& database) {
    GameplayVendorResult result;
    if (!active_) {
        return result;
    }
    pointer_x_ = input.pointer_x;
    pointer_y_ = input.pointer_y;
    updateHover(input.pointer_x, input.pointer_y, vendor);
    result.pointer_consumed =
        input.pointer_primary_pressed &&
        input.pointer_x >= 0 && input.pointer_x < 320 &&
        input.pointer_y >= 0 && input.pointer_y < 412;

    if (input.close_pressed) {
        if (held.heldItemFromVendor() &&
            held.heldVendorInventoryIndex() == inventory_index_) {
            if (std::optional<InventoryItem> item =
                    held.releaseHeldItem()) {
                if (!vendor.place(*item)) {
                    vendor.store(std::move(*item));
                }
            }
        }
        close();
        return result;
    }
    if (!input.pointer_primary_pressed ||
        input.pointer_x < 0 || input.pointer_x >= 320 ||
        input.pointer_y < 0 || input.pointer_y >= 412) {
        return result;
    }

    if (held.holdingItem()) {
        const InventoryItem* item = held.heldItem();
        if (!item) {
            return result;
        }
        const ItemDefinition* definition =
            database.find(item->category, item->definition_id);
        if (!definition) {
            return result;
        }
        if (held.heldItemFromVendor()) {
            if (held.heldVendorInventoryIndex() == inventory_index_) {
                std::optional<InventoryItem> returned =
                    held.releaseHeldItem();
                if (returned) {
                    if (!vendor.place(*returned)) {
                        vendor.store(std::move(*returned));
                    }
                    result.item_sound_sample =
                        retailItemMoveSound(*definition);
                }
            }
            return result;
        }
        if (item->category == 4 && item->definition_id == 0) {
            return result;
        }
        const std::int32_t price =
            itemSalePrice(*item, *definition);
        if (price > 0 && player.creditGold(price)) {
            held.releaseHeldItem();
            result.item_sound_sample =
                retailItemMoveSound(*definition);
        }
        return result;
    }

    if (hovered_item_index_ < 0 ||
        static_cast<std::size_t>(hovered_item_index_) >=
            vendor.items().size()) {
        return result;
    }
    const InventoryItem& selected = vendor.items()[
        static_cast<std::size_t>(hovered_item_index_)];
    const ItemDefinition* definition = database.find(
        selected.category, selected.definition_id);
    if (!definition) {
        return result;
    }
    const std::int32_t price =
        itemPurchasePrice(selected, *definition);
    if (price > player.gold()) {
        return result;
    }
    std::optional<InventoryItem> purchased = vendor.take(
        static_cast<std::size_t>(hovered_item_index_));
    if (purchased && held.holdVendorItem(
            std::move(*purchased), inventory_index_, price)) {
        result.item_sound_sample =
            retailItemMoveSound(*definition);
        hovered_item_index_ = -1;
        item_hover_updates_ = 0;
    } else if (purchased) {
        vendor.place(std::move(*purchased));
    }
    return result;
}

bool GameplayVendor::active() const {
    return active_;
}

std::int32_t GameplayVendor::inventoryIndex() const {
    return inventory_index_;
}

const InventoryItem* GameplayVendor::informationItem(
    const VendorInventory& inventory) const {
    if (!active_ || item_hover_updates_ < 3 ||
        hovered_item_index_ < 0 ||
        static_cast<std::size_t>(hovered_item_index_) >=
            inventory.items().size()) {
        return nullptr;
    }
    return &inventory.items()[
        static_cast<std::size_t>(hovered_item_index_)];
}

std::int32_t GameplayVendor::pointerX() const {
    return pointer_x_;
}

std::int32_t GameplayVendor::pointerY() const {
    return pointer_y_;
}

void GameplayVendor::updateHover(
    std::int32_t pointer_x,
    std::int32_t pointer_y,
    const VendorInventory& inventory) {
    std::int32_t next = -1;
    for (std::size_t index = 0;
         index < inventory.items().size();
         ++index) {
        const InventoryItem& item = inventory.items()[index];
        const std::int32_t left =
            item_left + item.grid_x * cell_size;
        const std::int32_t top =
            item_top + item.grid_y * cell_size;
        if (pointer_x >= left &&
            pointer_x < left + item.width * cell_size &&
            pointer_y >= top &&
            pointer_y < top + item.height * cell_size) {
            next = static_cast<std::int32_t>(index);
            break;
        }
    }
    if (next >= 0 && next == hovered_item_index_) {
        item_hover_updates_ =
            std::min(item_hover_updates_ + 1, 3);
    } else {
        item_hover_updates_ = next >= 0 ? 1 : 0;
    }
    hovered_item_index_ = next;
}

}  // namespace osf
