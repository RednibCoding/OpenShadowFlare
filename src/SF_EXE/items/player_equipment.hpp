#ifndef OPENSHADOWFLARE_PLAYER_EQUIPMENT_HPP
#define OPENSHADOWFLARE_PLAYER_EQUIPMENT_HPP

#include "player_inventory.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace osf {

class ItemDatabase;
struct ItemDefinition;

struct EquipmentPlacementResult {
    bool accepted = false;
    std::optional<InventoryItem> held_item;
};

class PlayerEquipment {
public:
    void clear();

    EquipmentPlacementResult placeMainHand(
        InventoryItem item,
        const ItemDefinition& definition,
        std::int32_t player_level);
    std::optional<InventoryItem> takeMainHand();

    const InventoryItem* mainHand() const;
    std::int32_t totalWeight(
        const ItemDatabase& database) const;
    std::int32_t derivedParameterBonus(
        std::size_t parameter,
        const ItemDatabase& database) const;

private:
    std::optional<InventoryItem> main_hand_;
};

}  // namespace osf

#endif
