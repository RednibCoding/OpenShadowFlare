#ifndef OPENSHADOWFLARE_PLAYER_INVENTORY_HPP
#define OPENSHADOWFLARE_PLAYER_INVENTORY_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace osf {

struct InventoryItem {
    std::int32_t category = -1;
    std::int32_t definition_id = -1;
    std::int32_t quantity = 1;
};

class PlayerInventory {
public:
    static constexpr std::int32_t maximum_gold_stack = 10000;

    void clear();
    bool add(
        std::int32_t category,
        std::int32_t definition_id,
        std::int32_t quantity = 1);

    const std::vector<InventoryItem>& items() const;

private:
    std::vector<InventoryItem> items_;
};

}  // namespace osf

#endif
