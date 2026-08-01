#ifndef OPENSHADOWFLARE_GAMEPLAY_INVENTORY_HPP
#define OPENSHADOWFLARE_GAMEPLAY_INVENTORY_HPP

#include "items/player_belt.hpp"
#include "items/player_equipment.hpp"

#include <cstdint>
#include <optional>

namespace osf {

class ItemDatabase;
class PlayerGiantWarehouse;
class PlayerSpecialItems;
struct GameplayInventoryInput {
    bool toggle_pressed = false;
    bool close_pressed = false;
    bool pointer_primary_pressed = false;
    std::int32_t pointer_x = 0;
    std::int32_t pointer_y = 0;
    bool special_toggle_pressed = false;
    bool pointer_secondary_pressed = false;
    bool identification_active = false;
};

struct GameplayInventoryResult {
    bool pointer_consumed = false;
    bool world_drop_requested = false;
    bool equipment_changed = false;
    std::int32_t inventory_item_use_requested = -1;
    std::int32_t belt_pocket_use_requested = -1;
    std::int32_t inventory_item_identify_requested = -1;
    bool cancel_identification_requested = false;
    std::int32_t world_drop_screen_x = 0;
    std::int32_t world_drop_screen_y = 0;
    std::int32_t item_sound_sample = -1;
};

struct EquipmentRegion {
    std::int32_t left = 0;
    std::int32_t top = 0;
    std::int32_t width_in_cells = 0;
    std::int32_t height_in_cells = 0;
};

struct BeltPocket {
    std::int32_t grid_x = 0;
    std::int32_t grid_y = 0;
};

class GameplayInventory {
public:
    static constexpr std::int32_t panel_left = 320;
    static constexpr std::int32_t backpack_left = 336;
    static constexpr std::int32_t backpack_top = 264;
    static constexpr std::int32_t special_left = 16;
    static constexpr std::int32_t special_top = 72;
    static constexpr std::int32_t cell_size = 32;
    static EquipmentRegion equipmentRegion(
        EquipmentSlot slot);
    static std::optional<BeltPocket> beltPocketAt(
        std::int32_t x,
        std::int32_t y);

    void open();
    void openSpecialItems();
    void openGiantWarehouse();
    void closeSpecialItems();
    void close();
    GameplayInventoryResult update(
        const GameplayInventoryInput& input,
        PlayerInventory& inventory,
        PlayerEquipment& equipment,
        PlayerBelt& belt,
        PlayerSpecialItems& special_items,
        const ItemDatabase& item_database,
        std::int32_t player_level,
        PlayerGiantWarehouse* giant_warehouse = nullptr);
    void completeWorldDrop(
        bool succeeded,
        PlayerInventory& inventory);
    void completeWorldDrop(bool succeeded);
    void completeItemUse(bool consumed);
    bool holdVendorItem(
        InventoryItem item,
        std::int32_t inventory_index,
        std::int32_t purchase_price);
    std::optional<InventoryItem> releaseHeldItem();
    bool heldItemFromVendor() const;
    std::int32_t heldVendorInventoryIndex() const;
    std::int32_t heldVendorPurchasePrice() const;

    bool active() const;
    bool specialItemsActive() const;
    bool giantWarehouseActive() const;
    bool leftStorageActive() const;
    bool anyItemPanelActive() const;
    bool closeHovered() const;
    std::int32_t hoveredItemIndex() const;
    std::optional<EquipmentSlot> hoveredEquipmentSlot() const;
    const InventoryItem* informationItem(
        const PlayerInventory& inventory,
        const PlayerEquipment& equipment,
        const PlayerSpecialItems& special_items,
        const PlayerGiantWarehouse* giant_warehouse = nullptr) const;
    bool holdingItem() const;
    const InventoryItem* heldItem() const;
    std::int32_t pointerX() const;
    std::int32_t pointerY() const;

private:
    enum class LeftStorage {
        none,
        special_items,
        giant_warehouse,
    };

    void updateHover(
        std::int32_t pointer_x,
        std::int32_t pointer_y,
        const PlayerInventory& inventory,
        const PlayerEquipment& equipment);
    void updateSpecialHover(
        std::int32_t pointer_x,
        std::int32_t pointer_y,
        const PlayerSpecialItems& special_items);
    void clearItemHover();
    void replaceHeldAfterPlacement(
        std::optional<InventoryItem> item,
        PlayerInventory& inventory);
    void markHeldAsPlayerItem();

    bool active_ = false;
    LeftStorage left_storage_ = LeftStorage::none;
    bool close_hovered_ = false;
    std::int32_t hovered_item_index_ = -1;
    std::int32_t hovered_equipment_slot_ = -1;
    std::int32_t hovered_special_item_index_ = -1;
    std::int32_t item_hover_updates_ = 0;
    std::optional<InventoryItem> held_item_;
    bool held_item_from_vendor_ = false;
    std::int32_t held_vendor_inventory_index_ = -1;
    std::int32_t held_vendor_purchase_price_ = 0;
    std::int32_t pointer_x_ = 0;
    std::int32_t pointer_y_ = 0;
};

}  // namespace osf

#endif
