#ifndef OPENSHADOWFLARE_PLAYER_SPECIAL_ITEMS_HPP
#define OPENSHADOWFLARE_PLAYER_SPECIAL_ITEMS_HPP

#include "player_inventory.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace osf {

class PlayerSpecialItems {
public:
    static constexpr std::int32_t grid_width = 9;
    static constexpr std::int32_t grid_height = 10;

    void clear();
    InventoryPlacementResult place(
        InventoryItem item,
        std::int32_t grid_x,
        std::int32_t grid_y);
    std::optional<InventoryItem> take(
        std::size_t item_index);

    const std::vector<InventoryItem>& items() const;
    bool contains(
        std::int32_t category,
        std::int32_t definition_id) const;

private:
    std::vector<InventoryItem> items_;
};

}  // namespace osf

#endif
