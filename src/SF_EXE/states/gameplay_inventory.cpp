#include "gameplay_inventory.hpp"

#include "items/item_audio.hpp"
#include "items/item_database.hpp"
#include "items/player_equipment.hpp"
#include "items/player_giant_warehouse.hpp"
#include "items/player_inventory.hpp"
#include "items/player_special_items.hpp"

#include <algorithm>
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

void setMoveSound(
    GameplayInventoryResult& result,
    const InventoryItem& item,
    const ItemDatabase& item_database) {
    const ItemDefinition* definition =
        item_database.find(
            item.category,
            item.definition_id);
    if (definition) {
        result.item_sound_sample =
            retailItemMoveSound(*definition);
    }
}

std::optional<EquipmentSlot> equipmentSlotAt(
    std::int32_t x,
    std::int32_t y) {
    for (std::size_t index = 0;
         index < PlayerEquipment::visible_slot_count;
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
    case EquipmentSlot::accessory_1:
        return {400, 143, 1, 1};
    case EquipmentSlot::accessory_2:
        return {400, 183, 1, 1};
    case EquipmentSlot::accessory_3:
        return {440, 143, 1, 1};
    case EquipmentSlot::accessory_4:
        return {440, 183, 1, 1};
    case EquipmentSlot::alternate_main_hand:
    case EquipmentSlot::alternate_off_hand:
    case EquipmentSlot::count:
        return {};
    }
    return {};
}

std::optional<BeltPocket> GameplayInventory::beltPocketAt(
    std::int32_t x,
    std::int32_t y) {
    // FUN_00445bd0 uses two staggered rows of four 32-pixel pockets.
    if (inside(x, y, 357, 413, 485, 445)) {
        return BeltPocket{(x - 357) / cell_size, 0};
    }
    if (inside(x, y, 405, 445, 533, 477)) {
        return BeltPocket{(x - 405) / cell_size, 1};
    }
    return std::nullopt;
}

void GameplayInventory::open() {
    active_ = true;
    close_hovered_ = false;
    clearItemHover();
}

void GameplayInventory::openSpecialItems() {
    left_storage_ = LeftStorage::special_items;
    close_hovered_ = false;
    clearItemHover();
}

void GameplayInventory::openGiantWarehouse() {
    left_storage_ = LeftStorage::giant_warehouse;
    close_hovered_ = false;
    clearItemHover();
}

void GameplayInventory::closeSpecialItems() {
    left_storage_ = LeftStorage::none;
    close_hovered_ = false;
    clearItemHover();
}

void GameplayInventory::close() {
    active_ = false;
    left_storage_ = LeftStorage::none;
    close_hovered_ = false;
    clearItemHover();
}

GameplayInventoryResult GameplayInventory::update(
    const GameplayInventoryInput& input,
    PlayerInventory& inventory,
    PlayerEquipment& equipment,
    PlayerBelt& belt,
    PlayerSpecialItems& special_items,
    const ItemDatabase& item_database,
    std::int32_t player_level,
    PlayerGiantWarehouse* giant_warehouse) {
    GameplayInventoryResult result;
    pointer_x_ = input.pointer_x;
    pointer_y_ = input.pointer_y;
    if (input.toggle_pressed) {
        if (active_) {
            active_ = false;
            close_hovered_ = false;
            clearItemHover();
        } else {
            open();
            updateHover(
                input.pointer_x,
                input.pointer_y,
                inventory,
                equipment);
        }
        return result;
    }
    if (input.special_toggle_pressed) {
        if (specialItemsActive()) {
            left_storage_ = LeftStorage::none;
            close_hovered_ = false;
            clearItemHover();
        } else {
            openSpecialItems();
            updateSpecialHover(
                input.pointer_x,
                input.pointer_y,
                special_items);
        }
        return result;
    }
    if (input.identification_active &&
        input.pointer_secondary_pressed) {
        result.pointer_consumed = true;
        result.cancel_identification_requested = true;
        return result;
    }
    if (input.identification_active &&
        input.pointer_primary_pressed) {
        result.pointer_consumed = true;
        if (active_ &&
            input.pointer_x >= panel_left &&
            input.pointer_y < 412) {
            updateHover(
                input.pointer_x,
                input.pointer_y,
                inventory,
                equipment);
            if (!held_item_ &&
                hovered_item_index_ >= 0 &&
                static_cast<std::size_t>(
                    hovered_item_index_) <
                    inventory.items().size() &&
                inventory.items()[
                    static_cast<std::size_t>(
                        hovered_item_index_)]
                        .identified == 0) {
                result.inventory_item_identify_requested =
                    hovered_item_index_;
            }
        }
        return result;
    }
    if (input.pointer_primary_pressed) {
        if (const std::optional<BeltPocket> pocket =
                beltPocketAt(
                    input.pointer_x,
                    input.pointer_y)) {
            if (held_item_) {
                const ItemDefinition* definition =
                    item_database.find(
                        held_item_->category,
                        held_item_->definition_id);
                if (definition) {
                    const InventoryPlacementResult placement =
                        belt.place(
                            *held_item_,
                            pocket->grid_x,
                            pocket->grid_y,
                            *definition);
                    if (placement.accepted) {
                        result.item_sound_sample =
                            retailItemMoveSound(
                                *definition);
                        replaceHeldAfterPlacement(
                            placement.held_item, inventory);
                    }
                }
            } else {
                held_item_ = belt.takeAt(
                    pocket->grid_x,
                    pocket->grid_y);
                markHeldAsPlayerItem();
                if (held_item_) {
                    setMoveSound(
                        result,
                        *held_item_,
                        item_database);
                }
            }
            result.pointer_consumed = true;
            return result;
        }
    }
    if (input.pointer_secondary_pressed && !held_item_) {
        if (const std::optional<BeltPocket> pocket =
                beltPocketAt(
                    input.pointer_x,
                    input.pointer_y)) {
            result.pointer_consumed = true;
            result.belt_pocket_use_requested =
                pocket->grid_y * PlayerBelt::grid_width +
                pocket->grid_x;
            return result;
        }
        if (active_) {
            updateHover(
                input.pointer_x,
                input.pointer_y,
                inventory,
                equipment);
            if (hovered_item_index_ >= 0) {
                result.pointer_consumed = true;
                result.inventory_item_use_requested =
                    hovered_item_index_;
                return result;
            }
        }
    }
    if (input.close_pressed) {
        close();
        return result;
    }
    PlayerSpecialItems* left_storage = nullptr;
    if (specialItemsActive()) {
        left_storage = &special_items;
    } else if (giantWarehouseActive() && giant_warehouse) {
        left_storage = &giant_warehouse->page(
            giant_warehouse->selectedPage());
    }
    if (left_storage &&
        (!active_ || input.pointer_x < panel_left)) {
        updateSpecialHover(
            input.pointer_x,
            input.pointer_y,
            *left_storage);
        if (input.pointer_primary_pressed) {
            if (giantWarehouseActive() &&
                inside(
                    input.pointer_x,
                    input.pointer_y,
                    272,
                    40,
                    296,
                    56)) {
                closeSpecialItems();
                result.pointer_consumed = true;
            } else if (giantWarehouseActive() &&
                       inside(
                           input.pointer_x,
                           input.pointer_y,
                           24,
                           40,
                           264,
                           56)) {
                const std::size_t page =
                    static_cast<std::size_t>(
                        (input.pointer_x - 24) / 24);
                if (giant_warehouse &&
                    giant_warehouse->selectPage(page)) {
                    result.item_sound_sample = 58;
                    left_storage = &giant_warehouse->page(page);
                    updateSpecialHover(
                        input.pointer_x,
                        input.pointer_y,
                        *left_storage);
                }
                result.pointer_consumed = true;
            } else if (inside(
                    input.pointer_x,
                    input.pointer_y,
                    special_left,
                    special_top,
                    special_left +
                        PlayerSpecialItems::grid_width *
                            cell_size,
                    special_top +
                        PlayerSpecialItems::grid_height *
                            cell_size)) {
                if (held_item_) {
                    const InventoryItem& item = *held_item_;
                    const std::int32_t grid_x =
                        (input.pointer_x -
                         item.width * cell_size / 2) /
                        cell_size;
                    const std::int32_t grid_y =
                        (input.pointer_y -
                         item.height * cell_size / 2 -
                         (special_top - cell_size / 2)) /
                        cell_size;
                    const InventoryPlacementResult placement =
                        left_storage->place(
                            item,
                            grid_x,
                            grid_y);
                    if (placement.accepted) {
                        setMoveSound(
                            result,
                            item,
                            item_database);
                        replaceHeldAfterPlacement(
                            placement.held_item, inventory);
                    }
                } else if (
                    hovered_special_item_index_ >= 0) {
                    held_item_ = left_storage->take(
                        static_cast<std::size_t>(
                            hovered_special_item_index_));
                    markHeldAsPlayerItem();
                    if (held_item_) {
                        setMoveSound(
                            result,
                            *held_item_,
                            item_database);
                    }
                    hovered_special_item_index_ = -1;
                    item_hover_updates_ = 0;
                }
                result.pointer_consumed = true;
            } else if (
                holdingItem() &&
                input.pointer_x >= panel_left &&
                input.pointer_y < 412) {
                result.pointer_consumed = true;
                result.world_drop_requested = true;
                result.world_drop_screen_x =
                    input.pointer_x;
                result.world_drop_screen_y =
                    input.pointer_y;
            } else if (
                input.pointer_x < panel_left &&
                input.pointer_y < 412) {
                result.pointer_consumed = true;
            }
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
        inventory,
        equipment);
    if (
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
                        result.item_sound_sample =
                            retailItemEquipSound(
                                *definition);
                        replaceHeldAfterPlacement(
                            std::move(placement.held_item),
                            inventory);
                        result.equipment_changed = true;
                    }
                }
            } else {
                held_item_ = equipment.take(*slot);
                markHeldAsPlayerItem();
                result.equipment_changed =
                    held_item_.has_value();
                if (held_item_) {
                    setMoveSound(
                        result,
                        *held_item_,
                        item_database);
                }
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
                    setMoveSound(
                        result,
                        item,
                        item_database);
                    replaceHeldAfterPlacement(
                        placement.held_item, inventory);
                }
            }
            result.pointer_consumed = true;
        } else if (
            !holdingItem() &&
            hovered_item_index_ >= 0) {
            held_item_ = inventory.take(
                static_cast<std::size_t>(
                    hovered_item_index_));
            markHeldAsPlayerItem();
            if (held_item_) {
                setMoveSound(
                    result,
                    *held_item_,
                    item_database);
            }
            hovered_item_index_ = -1;
            item_hover_updates_ = 0;
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
    bool succeeded,
    PlayerInventory& inventory) {
    if (succeeded) {
        if (held_item_from_vendor_) {
            inventory.spendGold(
                held_vendor_purchase_price_);
        }
        held_item_.reset();
        markHeldAsPlayerItem();
    }
}

void GameplayInventory::completeWorldDrop(bool succeeded) {
    if (succeeded && !held_item_from_vendor_) {
        held_item_.reset();
        markHeldAsPlayerItem();
    }
}

bool GameplayInventory::holdVendorItem(
    InventoryItem item,
    std::int32_t inventory_index,
    std::int32_t purchase_price) {
    if (held_item_ || inventory_index < 0 || purchase_price < 0) {
        return false;
    }
    held_item_ = std::move(item);
    held_item_from_vendor_ = true;
    held_vendor_inventory_index_ = inventory_index;
    held_vendor_purchase_price_ = purchase_price;
    return true;
}

std::optional<InventoryItem> GameplayInventory::releaseHeldItem() {
    std::optional<InventoryItem> item = std::move(held_item_);
    held_item_.reset();
    markHeldAsPlayerItem();
    return item;
}

bool GameplayInventory::heldItemFromVendor() const {
    return held_item_from_vendor_;
}

std::int32_t GameplayInventory::heldVendorInventoryIndex() const {
    return held_vendor_inventory_index_;
}

std::int32_t GameplayInventory::heldVendorPurchasePrice() const {
    return held_vendor_purchase_price_;
}

void GameplayInventory::replaceHeldAfterPlacement(
    std::optional<InventoryItem> item,
    PlayerInventory& inventory) {
    if (held_item_from_vendor_) {
        inventory.spendGold(held_vendor_purchase_price_);
    }
    held_item_ = std::move(item);
    markHeldAsPlayerItem();
}

void GameplayInventory::markHeldAsPlayerItem() {
    held_item_from_vendor_ = false;
    held_vendor_inventory_index_ = -1;
    held_vendor_purchase_price_ = 0;
}

void GameplayInventory::completeItemUse(
    bool consumed) {
    if (consumed) {
        clearItemHover();
    }
}

bool GameplayInventory::active() const {
    return active_;
}

bool GameplayInventory::specialItemsActive() const {
    return left_storage_ == LeftStorage::special_items;
}

bool GameplayInventory::giantWarehouseActive() const {
    return left_storage_ == LeftStorage::giant_warehouse;
}

bool GameplayInventory::leftStorageActive() const {
    return left_storage_ != LeftStorage::none;
}

bool GameplayInventory::anyItemPanelActive() const {
    return active_ || leftStorageActive();
}

bool GameplayInventory::closeHovered() const {
    return close_hovered_;
}

std::int32_t GameplayInventory::hoveredItemIndex() const {
    return hovered_item_index_;
}

std::optional<EquipmentSlot>
GameplayInventory::hoveredEquipmentSlot() const {
    if (hovered_equipment_slot_ < 0) {
        return std::nullopt;
    }
    return static_cast<EquipmentSlot>(
        hovered_equipment_slot_);
}

const InventoryItem* GameplayInventory::informationItem(
    const PlayerInventory& inventory,
    const PlayerEquipment& equipment,
    const PlayerSpecialItems& special_items,
    const PlayerGiantWarehouse* giant_warehouse) const {
    // FUN_00408a80 waits for three cursor updates before creating the
    // information display and does not describe an item being carried.
    if (!anyItemPanelActive() ||
        held_item_ ||
        item_hover_updates_ < 3) {
        return nullptr;
    }
    if (hovered_special_item_index_ >= 0) {
        const PlayerSpecialItems* storage = &special_items;
        if (giantWarehouseActive() && giant_warehouse) {
            storage = &giant_warehouse->page(
                giant_warehouse->selectedPage());
        }
        if (hovered_special_item_index_ < 0 ||
            static_cast<std::size_t>(
                hovered_special_item_index_) >=
                storage->items().size()) {
            return nullptr;
        }
        return &storage->items()[
            static_cast<std::size_t>(
                hovered_special_item_index_)];
    }
    if (hovered_equipment_slot_ >= 0) {
        return equipment.item(
            static_cast<EquipmentSlot>(
                hovered_equipment_slot_));
    }
    if (hovered_item_index_ < 0 ||
        static_cast<std::size_t>(hovered_item_index_) >=
            inventory.items().size()) {
        return nullptr;
    }
    return &inventory.items()[
        static_cast<std::size_t>(hovered_item_index_)];
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
    const PlayerInventory& inventory,
    const PlayerEquipment& equipment) {
    close_hovered_ = inside(
        pointer_x,
        pointer_y,
        kCloseLeft,
        kCloseTop,
        kCloseRight,
        kCloseBottom);

    std::int32_t next_item_index = -1;
    std::int32_t next_equipment_slot = -1;
    if (held_item_) {
        clearItemHover();
        return;
    }
    if (const std::optional<EquipmentSlot> slot =
            equipmentSlotAt(pointer_x, pointer_y);
        slot && equipment.item(*slot)) {
        next_equipment_slot =
            static_cast<std::int32_t>(*slot);
    }
    const auto& items = inventory.items();
    if (next_equipment_slot < 0) {
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
                next_item_index =
                    static_cast<std::int32_t>(index);
                break;
            }
        }
    }
    if (next_item_index == hovered_item_index_ &&
        next_equipment_slot == hovered_equipment_slot_ &&
        (next_item_index >= 0 ||
         next_equipment_slot >= 0)) {
        item_hover_updates_ =
            std::min(item_hover_updates_ + 1, 3);
    } else {
        item_hover_updates_ =
            next_item_index >= 0 ||
                    next_equipment_slot >= 0
                ? 1
                : 0;
    }
    hovered_item_index_ = next_item_index;
    hovered_equipment_slot_ = next_equipment_slot;
    hovered_special_item_index_ = -1;
}

void GameplayInventory::updateSpecialHover(
    std::int32_t pointer_x,
    std::int32_t pointer_y,
    const PlayerSpecialItems& special_items) {
    close_hovered_ = false;
    std::int32_t next_special_item_index = -1;
    if (!held_item_) {
        const auto& items = special_items.items();
        for (std::size_t index = 0;
             index < items.size();
             ++index) {
            const InventoryItem& item = items[index];
            if (inside(
                    pointer_x,
                    pointer_y,
                    special_left + item.grid_x * cell_size,
                    special_top + item.grid_y * cell_size,
                    special_left +
                        (item.grid_x + item.width) * cell_size,
                    special_top +
                        (item.grid_y + item.height) * cell_size)) {
                next_special_item_index =
                    static_cast<std::int32_t>(index);
                break;
            }
        }
    }
    if (next_special_item_index ==
            hovered_special_item_index_ &&
        next_special_item_index >= 0) {
        item_hover_updates_ =
            std::min(item_hover_updates_ + 1, 3);
    } else {
        item_hover_updates_ =
            next_special_item_index >= 0 ? 1 : 0;
    }
    hovered_item_index_ = -1;
    hovered_equipment_slot_ = -1;
    hovered_special_item_index_ =
        next_special_item_index;
}

void GameplayInventory::clearItemHover() {
    hovered_item_index_ = -1;
    hovered_equipment_slot_ = -1;
    hovered_special_item_index_ = -1;
    item_hover_updates_ = 0;
}

}  // namespace osf
