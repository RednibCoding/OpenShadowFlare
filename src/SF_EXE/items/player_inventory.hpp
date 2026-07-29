#ifndef OPENSHADOWFLARE_PLAYER_INVENTORY_HPP
#define OPENSHADOWFLARE_PLAYER_INVENTORY_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace osf {

struct ItemDefinition;

struct InventoryItem {
    std::int32_t category = -1;
    std::int32_t definition_id = -1;
    std::int32_t quantity = 1;
    std::int32_t grid_x = 0;
    std::int32_t grid_y = 0;
    std::int32_t width = 1;
    std::int32_t height = 1;
};

struct InventoryPlacementResult {
    bool accepted = false;
    std::optional<InventoryItem> held_item;
};

class PlayerInventory {
public:
    static constexpr std::int32_t maximum_gold_stack = 10000;
    static constexpr std::int32_t grid_width = 9;
    static constexpr std::int32_t grid_height = 4;

    void clear();
    bool add(
        std::int32_t category,
        std::int32_t definition_id,
        std::int32_t quantity = 1);
    bool add(
        const ItemDefinition& definition,
        std::int32_t quantity = 1);
    std::optional<InventoryItem> take(
        std::size_t item_index);
    InventoryPlacementResult place(
        InventoryItem item,
        std::int32_t grid_x,
        std::int32_t grid_y);

    const std::vector<InventoryItem>& items() const;
    std::int32_t gold() const;

private:
    bool add(
        std::int32_t category,
        std::int32_t definition_id,
        std::int32_t quantity,
        std::int32_t width,
        std::int32_t height);

    std::vector<InventoryItem> items_;
};

}  // namespace osf

#endif
