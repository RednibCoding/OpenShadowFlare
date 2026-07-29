#include "player_equipment.hpp"

#include "item_database.hpp"

#include <utility>

namespace osf {

void PlayerEquipment::clear() {
    for (auto& slot : slots_) {
        slot.reset();
    }
}

EquipmentPlacementResult PlayerEquipment::place(
    EquipmentSlot slot,
    InventoryItem item,
    const ItemDefinition& definition,
    std::int32_t player_level) {
    // FUN_00446320 classifies the five ordinary regions by category and
    // armor subtype, then applies the same required-level gate to each.
    if (!accepts(slot, definition) ||
        definition.id != item.definition_id ||
        item.category != definition.category ||
        item.quantity != 1 ||
        player_level < definition.required_level) {
        return {};
    }

    EquipmentPlacementResult result;
    result.accepted = true;
    std::optional<InventoryItem>& destination =
        slots_[static_cast<std::size_t>(slot)];
    result.held_item = std::move(destination);
    item.grid_x = 0;
    item.grid_y = 0;
    destination = std::move(item);
    return result;
}

std::optional<InventoryItem> PlayerEquipment::take(
    EquipmentSlot slot) {
    const std::size_t index =
        static_cast<std::size_t>(slot);
    if (index >= slot_count) {
        return std::nullopt;
    }
    std::optional<InventoryItem>& source =
        slots_[index];
    std::optional<InventoryItem> item =
        std::move(source);
    source.reset();
    return item;
}

const InventoryItem* PlayerEquipment::item(
    EquipmentSlot slot) const {
    const std::size_t index =
        static_cast<std::size_t>(slot);
    if (index >= slot_count) {
        return nullptr;
    }
    const std::optional<InventoryItem>& equipped =
        slots_[index];
    return equipped ? &*equipped : nullptr;
}

std::int32_t PlayerEquipment::totalWeight(
    const ItemDatabase& database) const {
    std::int32_t weight = 0;
    for (const auto& equipped : slots_) {
        if (!equipped) {
            continue;
        }
        const ItemDefinition* definition =
            database.find(
                equipped->category,
                equipped->definition_id);
        if (definition) {
            weight +=
                definition->weight * equipped->quantity;
        }
    }
    return weight;
}

std::int32_t PlayerEquipment::derivedParameterBonus(
    std::size_t parameter,
    const ItemDatabase& database) const {
    if (parameter >= ItemDefinition::derived_parameter_count) {
        return 0;
    }
    std::int32_t bonus = 0;
    for (const auto& equipped : slots_) {
        if (!equipped) {
            continue;
        }
        const ItemDefinition* definition =
            database.find(
                equipped->category,
                equipped->definition_id);
        if (definition) {
            bonus +=
                definition->derived_parameter_bonuses[parameter];
        }
    }
    return bonus;
}

bool PlayerEquipment::accepts(
    EquipmentSlot slot,
    const ItemDefinition& definition) {
    if (slot == EquipmentSlot::main_hand) {
        return definition.category == 0;
    }
    if (definition.category != 1) {
        return false;
    }
    switch (slot) {
    case EquipmentSlot::helmet:
        return definition.subtype == 0;
    case EquipmentSlot::body:
        return definition.subtype == 1;
    case EquipmentSlot::boots:
        return definition.subtype == 3;
    case EquipmentSlot::off_hand:
        return definition.subtype == 2;
    case EquipmentSlot::main_hand:
    case EquipmentSlot::count:
        return false;
    }
    return false;
}

}  // namespace osf
