#include "gameplay_inventory.hpp"

#include "items/item_database.hpp"
#include "items/player_equipment.hpp"
#include "items/player_inventory.hpp"

#include <cstddef>
#include <utility>

namespace osf {
namespace {

constexpr std::int32_t kCloseLeft = 375;
constexpr std::int32_t kCloseTop = 393;
constexpr std::int32_t kCloseRight = 443;
constexpr std::int32_t kCloseBottom = 404;
constexpr std::int32_t kBackpackRight =
    GameplayInventory::backpack_left +
    PlayerInventory::grid_width *
        GameplayInventory::cell_size;
constexpr std::int32_t kBackpackBottom =
    GameplayInventory::backpack_top +
    PlayerInventory::grid_height *
        GameplayInventory::cell_size;

bool inside(
    std::int32_t x,
    std::int32_t y,
    std::int32_t left,
    std::int32_t top,
    std::int32_t right,
    std::int32_t bottom) {
    return x >= left && x < right &&
           y >= top && y < bottom;
}

std::optional<EquipmentSlot> equipmentSlotAt(
    std::int32_t x,
    std::int32_t y) {
    for (std::size_t index = 0;
         index < PlayerEquipment::slot_count;
         ++index) {
        const EquipmentSlot slot =
            static_cast<EquipmentSlot>(index);
        const EquipmentRegion region =
            GameplayInventory::equipmentRegion(slot);
        if (inside(
                x,
                y,
                region.left,
                region.top,
                region.left +
                    region.width_in_cells *
                        GameplayInventory::cell_size,
                region.top +
                    region.height_in_cells *
                        GameplayInventory::cell_size)) {
            return slot;
        }
    }
    return std::nullopt;
}

}  // namespace

EquipmentRegion GameplayInventory::equipmentRegion(
    EquipmentSlot slot) {
    switch (slot) {
    case EquipmentSlot::helmet:
        return {560, 16, 2, 2};
    case EquipmentSlot::body:
        return {560, 88, 2, 3};
    case EquipmentSlot::boots:
        return {560, 192, 2, 2};
    case EquipmentSlot::main_hand:
        return {480, 16, 2, 4};
    case EquipmentSlot::off_hand:
        return {480, 160, 2, 3};
    case EquipmentSlot::count:
        return {};
    }
    return {};
}

void GameplayInventory::open() {
    active_ = true;
    close_hovered_ = false;
    hovered_item_index_ = -1;
}

void GameplayInventory::close() {
    active_ = false;
    close_hovered_ = false;
    hovered_item_index_ = -1;
}

GameplayInventoryResult GameplayInventory::update(
    const GameplayInventoryInput& input,
    PlayerInventory& inventory,
    PlayerEquipment& equipment,
    const ItemDatabase& item_database,
    std::int32_t player_level) {
    GameplayInventoryResult result;
    pointer_x_ = input.pointer_x;
    pointer_y_ = input.pointer_y;
    if (input.toggle_pressed) {
        if (active_) {
            close();
        } else {
            open();
            updateHover(
                input.pointer_x,
                input.pointer_y,
                inventory);
        }
        return result;
    }
    if (!active_) {
        if (holdingItem() &&
            input.pointer_primary_pressed &&
            input.pointer_y < 412) {
            result.pointer_consumed = true;
            result.world_drop_requested = true;
            result.world_drop_screen_x = input.pointer_x;
            result.world_drop_screen_y = input.pointer_y;
        }
        return result;
    }

    updateHover(
        input.pointer_x,
        input.pointer_y,
        inventory);
    if (input.close_pressed) {
        close();
    } else if (
        input.pointer_primary_pressed &&
        close_hovered_) {
        result.pointer_consumed =
            true;
        close();
    } else if (input.pointer_primary_pressed) {
        if (holdingItem() &&
            input.pointer_x < panel_left &&
            input.pointer_y < 412) {
            result.pointer_consumed = true;
            result.world_drop_requested = true;
            result.world_drop_screen_x = input.pointer_x;
            result.world_drop_screen_y = input.pointer_y;
        } else if (const std::optional<EquipmentSlot> slot =
                       equipmentSlotAt(
                           input.pointer_x,
                           input.pointer_y)) {
            if (held_item_) {
                const ItemDefinition* definition =
                    item_database.find(
                        held_item_->category,
                        held_item_->definition_id);
                if (definition) {
                    EquipmentPlacementResult placement =
                        equipment.place(
                            *slot,
                            *held_item_,
                            *definition,
                            player_level);
                    if (placement.accepted) {
                        held_item_ =
                            std::move(placement.held_item);
                        result.equipment_changed = true;
                    }
                }
            } else {
                held_item_ = equipment.take(*slot);
                result.equipment_changed =
                    held_item_.has_value();
            }
            result.pointer_consumed = true;
        } else if (
            holdingItem() &&
            inside(
                input.pointer_x,
                input.pointer_y,
                backpack_left,
                backpack_top,
                kBackpackRight,
                kBackpackBottom)) {
            if (held_item_) {
                const InventoryItem& item = *held_item_;
                // FUN_00446320 centers the full icon on the
                // pointer, then rounds its top-left corner to
                // the nearest backpack cell.
                const std::int32_t grid_x =
                    (input.pointer_x -
                     item.width * cell_size / 2 -
                     (backpack_left - cell_size / 2)) /
                    cell_size;
                const std::int32_t grid_y =
                    (input.pointer_y -
                     item.height * cell_size / 2 -
                     (backpack_top - cell_size / 2)) /
                    cell_size;
                const InventoryPlacementResult placement =
                    inventory.place(
                        item,
                        grid_x,
                        grid_y);
                if (placement.accepted) {
                    held_item_ =
                        placement.held_item;
                }
            }
            result.pointer_consumed = true;
        } else if (
            !holdingItem() &&
            hovered_item_index_ >= 0) {
            held_item_ = inventory.take(
                static_cast<std::size_t>(
                    hovered_item_index_));
            hovered_item_index_ = -1;
            result.pointer_consumed = true;
        } else if (
            input.pointer_x >= panel_left &&
            input.pointer_y < 412) {
            result.pointer_consumed = true;
        }
    }
    return result;
}

void GameplayInventory::completeWorldDrop(
    bool succeeded) {
    if (succeeded) {
        held_item_.reset();
    }
}

bool GameplayInventory::active() const {
    return active_;
}

bool GameplayInventory::closeHovered() const {
    return close_hovered_;
}

std::int32_t GameplayInventory::hoveredItemIndex() const {
    return hovered_item_index_;
}

bool GameplayInventory::holdingItem() const {
    return held_item_.has_value();
}

const InventoryItem* GameplayInventory::heldItem() const {
    return held_item_ ? &*held_item_ : nullptr;
}

std::int32_t GameplayInventory::pointerX() const {
    return pointer_x_;
}

std::int32_t GameplayInventory::pointerY() const {
    return pointer_y_;
}

void GameplayInventory::updateHover(
    std::int32_t pointer_x,
    std::int32_t pointer_y,
    const PlayerInventory& inventory) {
    close_hovered_ = inside(
        pointer_x,
        pointer_y,
        kCloseLeft,
        kCloseTop,
        kCloseRight,
        kCloseBottom);
    hovered_item_index_ = -1;
    const auto& items = inventory.items();
    for (std::size_t index = 0;
         index < items.size();
         ++index) {
        const InventoryItem& item = items[index];
        if (inside(
                pointer_x,
                pointer_y,
                backpack_left + item.grid_x * cell_size,
                backpack_top + item.grid_y * cell_size,
                backpack_left +
                    (item.grid_x + item.width) * cell_size,
                backpack_top +
                    (item.grid_y + item.height) * cell_size)) {
            hovered_item_index_ =
                static_cast<std::int32_t>(index);
            break;
        }
    }
}

}  // namespace osf
