#ifndef OPENSHADOWFLARE_VENDOR_INVENTORY_HPP
#define OPENSHADOWFLARE_VENDOR_INVENTORY_HPP

#include "player_inventory.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace osf {

class VendorInventory {
public:
    static constexpr std::int32_t grid_width = 9;
    static constexpr std::int32_t grid_height = 10;

    void clear();
    bool store(InventoryItem item);
    bool store(
        InventoryItem item,
        std::int32_t start_x,
        std::int32_t start_y);
    bool place(InventoryItem item);
    std::optional<InventoryItem> take(std::size_t item_index);
    const InventoryItem* itemAt(
        std::int32_t grid_x,
        std::int32_t grid_y) const;
    const std::vector<InventoryItem>& items() const;

private:
    std::vector<InventoryItem> items_;
};

}  // namespace osf

#endif
