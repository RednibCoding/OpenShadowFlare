#ifndef OPENSHADOWFLARE_GAMEPLAY_INVENTORY_HPP
#define OPENSHADOWFLARE_GAMEPLAY_INVENTORY_HPP

#include "items/player_equipment.hpp"

#include <cstdint>
#include <optional>

namespace osf {

class ItemDatabase;
struct GameplayInventoryInput {
    bool toggle_pressed = false;
    bool close_pressed = false;
    bool pointer_primary_pressed = false;
    std::int32_t pointer_x = 0;
    std::int32_t pointer_y = 0;
};

struct GameplayInventoryResult {
    bool pointer_consumed = false;
    bool world_drop_requested = false;
    bool equipment_changed = false;
    std::int32_t world_drop_screen_x = 0;
    std::int32_t world_drop_screen_y = 0;
};

struct EquipmentRegion {
    std::int32_t left = 0;
    std::int32_t top = 0;
    std::int32_t width_in_cells = 0;
    std::int32_t height_in_cells = 0;
};

class GameplayInventory {
public:
    static constexpr std::int32_t panel_left = 320;
    static constexpr std::int32_t backpack_left = 336;
    static constexpr std::int32_t backpack_top = 264;
    static constexpr std::int32_t cell_size = 32;
    static EquipmentRegion equipmentRegion(
        EquipmentSlot slot);

    void open();
    void close();
    GameplayInventoryResult update(
        const GameplayInventoryInput& input,
        PlayerInventory& inventory,
        PlayerEquipment& equipment,
        const ItemDatabase& item_database,
        std::int32_t player_level);
    void completeWorldDrop(bool succeeded);

    bool active() const;
    bool closeHovered() const;
    std::int32_t hoveredItemIndex() const;
    bool holdingItem() const;
    const InventoryItem* heldItem() const;
    std::int32_t pointerX() const;
    std::int32_t pointerY() const;

private:
    void updateHover(
        std::int32_t pointer_x,
        std::int32_t pointer_y,
        const PlayerInventory& inventory);

    bool active_ = false;
    bool close_hovered_ = false;
    std::int32_t hovered_item_index_ = -1;
    std::optional<InventoryItem> held_item_;
    std::int32_t pointer_x_ = 0;
    std::int32_t pointer_y_ = 0;
};

}  // namespace osf

#endif
