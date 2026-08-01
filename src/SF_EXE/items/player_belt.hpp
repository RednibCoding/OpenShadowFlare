#ifndef OPENSHADOWFLARE_PLAYER_BELT_HPP
#define OPENSHADOWFLARE_PLAYER_BELT_HPP

#include "player_inventory.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace osf {

struct ItemDefinition;

class PlayerBelt {
public:
    static constexpr std::int32_t grid_width = 4;
    static constexpr std::int32_t grid_height = 2;

    void clear();
    InventoryPlacementResult place(
        InventoryItem item,
        std::int32_t grid_x,
        std::int32_t grid_y,
        const ItemDefinition& definition);
    std::optional<InventoryItem> takeAt(
        std::int32_t grid_x,
        std::int32_t grid_y);

    const InventoryItem* itemAt(
        std::int32_t grid_x,
        std::int32_t grid_y) const;
    const std::vector<InventoryItem>& items() const;
    std::int32_t identifyAll();
    bool hasUnidentifiedItems() const;

private:
    std::vector<InventoryItem> items_;
};

}  // namespace osf

#endif
