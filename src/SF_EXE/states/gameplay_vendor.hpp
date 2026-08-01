#ifndef OPENSHADOWFLARE_GAMEPLAY_VENDOR_HPP
#define OPENSHADOWFLARE_GAMEPLAY_VENDOR_HPP

#include <cstdint>

namespace osf {

class GameplayInventory;
class ItemDatabase;
class PlayerInventory;
class VendorInventory;
struct InventoryItem;

struct GameplayVendorInput {
    bool close_pressed = false;
    bool pointer_primary_pressed = false;
    std::int32_t pointer_x = 0;
    std::int32_t pointer_y = 0;
};

struct GameplayVendorResult {
    bool pointer_consumed = false;
    std::int32_t item_sound_sample = -1;
};

class GameplayVendor {
public:
    static constexpr std::int32_t item_left = 16;
    static constexpr std::int32_t item_top = 72;
    static constexpr std::int32_t cell_size = 32;

    void open(std::int32_t inventory_index);
    void close();
    GameplayVendorResult update(
        const GameplayVendorInput& input,
        VendorInventory& vendor,
        GameplayInventory& held,
        PlayerInventory& player,
        const ItemDatabase& database);

    bool active() const;
    std::int32_t inventoryIndex() const;
    const InventoryItem* informationItem(
        const VendorInventory& inventory) const;
    std::int32_t pointerX() const;
    std::int32_t pointerY() const;

private:
    void updateHover(
        std::int32_t pointer_x,
        std::int32_t pointer_y,
        const VendorInventory& inventory);

    bool active_ = false;
    std::int32_t inventory_index_ = -1;
    std::int32_t hovered_item_index_ = -1;
    std::int32_t item_hover_updates_ = 0;
    std::int32_t pointer_x_ = 0;
    std::int32_t pointer_y_ = 0;
};

}  // namespace osf

#endif
