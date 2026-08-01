#ifndef OPENSHADOWFLARE_PLAYER_INVENTORY_HPP
#define OPENSHADOWFLARE_PLAYER_INVENTORY_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace osf {

struct ItemDefinition;
class ItemDatabase;
class TableData;

struct InventoryItem {
    std::int32_t category = -1;
    std::int32_t definition_id = -1;
    std::int32_t quantity = 1;
    std::int32_t grid_x = 0;
    std::int32_t grid_y = 0;
    std::int32_t width = 1;
    std::int32_t height = 1;
    std::int32_t durability = -1;
    std::int32_t identified = 0;
    // Preserves the category-specific retail instance fields that have not
    // been named yet. Save/load patches the fields represented above.
    std::vector<std::uint8_t> retail_state;
};

struct InventoryPlacementResult {
    bool accepted = false;
    std::optional<InventoryItem> held_item;
};

InventoryItem makeInventoryItem(
    const ItemDefinition& definition,
    std::int32_t quantity = 1);
bool identifyInventoryItem(InventoryItem& item);

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
    bool store(InventoryItem item);
    std::optional<InventoryItem> take(
        std::size_t item_index);
    bool identify(std::size_t item_index);
    std::int32_t identifyAll();
    bool hasUnidentifiedItems() const;
    std::int32_t repairPrice(
        const ItemDatabase& database,
        const TableData& value_parameters) const;
    std::int32_t repairAll(const ItemDatabase& database);
    InventoryPlacementResult place(
        InventoryItem item,
        std::int32_t grid_x,
        std::int32_t grid_y);

    const InventoryItem* itemAt(
        std::int32_t grid_x,
        std::int32_t grid_y) const;
    const std::vector<InventoryItem>& items() const;
    std::int32_t gold() const;
    bool spendGold(std::int32_t amount);
    bool creditGold(std::int32_t amount);

private:
    bool add(
        std::int32_t category,
        std::int32_t definition_id,
        std::int32_t quantity,
        std::int32_t width,
        std::int32_t height,
        std::int32_t durability,
        std::int32_t identified);

    std::vector<InventoryItem> items_;
};

}  // namespace osf

#endif
