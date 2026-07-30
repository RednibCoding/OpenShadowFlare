#ifndef OPENSHADOWFLARE_PLAYER_EQUIPMENT_HPP
#define OPENSHADOWFLARE_PLAYER_EQUIPMENT_HPP

#include "player_inventory.hpp"

#include <array>
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

enum class EquipmentSlot : std::size_t {
    helmet,
    body,
    boots,
    main_hand,
    off_hand,
    accessory_1,
    accessory_2,
    accessory_3,
    accessory_4,
    count
};

class PlayerEquipment {
public:
    static constexpr std::size_t slot_count =
        static_cast<std::size_t>(EquipmentSlot::count);

    void clear();

    EquipmentPlacementResult place(
        EquipmentSlot slot,
        InventoryItem item,
        const ItemDefinition& definition,
        std::int32_t player_level);
    std::optional<InventoryItem> take(EquipmentSlot slot);

    const InventoryItem* item(EquipmentSlot slot) const;
    bool decreaseDurability(
        EquipmentSlot slot,
        std::int32_t amount);
    std::int32_t totalWeight(
        const ItemDatabase& database) const;
    std::int32_t derivedParameterBonus(
        std::size_t parameter,
        const ItemDatabase& database) const;

private:
    static bool accepts(
        EquipmentSlot slot,
        const ItemDefinition& definition);

    std::array<
        std::optional<InventoryItem>,
        slot_count> slots_;
};

}  // namespace osf

#endif
