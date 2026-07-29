#ifndef OPENSHADOWFLARE_GAMEPLAY_INVENTORY_HPP
#define OPENSHADOWFLARE_GAMEPLAY_INVENTORY_HPP

#include <cstdint>

namespace osf {

class PlayerInventory;

struct GameplayInventoryInput {
    bool toggle_pressed = false;
    bool close_pressed = false;
    bool pointer_primary_pressed = false;
    std::int32_t pointer_x = 0;
    std::int32_t pointer_y = 0;
};

struct GameplayInventoryResult {
    bool pointer_consumed = false;
};

class GameplayInventory {
public:
    static constexpr std::int32_t panel_left = 320;
    static constexpr std::int32_t backpack_left = 336;
    static constexpr std::int32_t backpack_top = 264;
    static constexpr std::int32_t cell_size = 32;

    void open();
    void close();
    GameplayInventoryResult update(
        const GameplayInventoryInput& input,
        const PlayerInventory& inventory);

    bool active() const;
    bool closeHovered() const;
    std::int32_t hoveredItemIndex() const;

private:
    void updateHover(
        std::int32_t pointer_x,
        std::int32_t pointer_y,
        const PlayerInventory& inventory);

    bool active_ = false;
    bool close_hovered_ = false;
    std::int32_t hovered_item_index_ = -1;
};

}  // namespace osf

#endif
