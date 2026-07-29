#include "player_equipment.hpp"

#include "item_database.hpp"

#include <utility>

namespace osf {

void PlayerEquipment::clear() {
    main_hand_.reset();
}

EquipmentPlacementResult PlayerEquipment::placeMainHand(
    InventoryItem item,
    const ItemDefinition& definition,
    std::int32_t player_level) {
    // FUN_00446320 accepts category zero in the main-hand region and
    // compares the item's required level with the player's current level.
    if (definition.category != 0 ||
        definition.id != item.definition_id ||
        item.category != definition.category ||
        item.quantity != 1 ||
        player_level < definition.required_level) {
        return {};
    }

    EquipmentPlacementResult result;
    result.accepted = true;
    result.held_item = std::move(main_hand_);
    item.grid_x = 0;
    item.grid_y = 0;
    main_hand_ = std::move(item);
    return result;
}

std::optional<InventoryItem>
PlayerEquipment::takeMainHand() {
    std::optional<InventoryItem> item =
        std::move(main_hand_);
    main_hand_.reset();
    return item;
}

const InventoryItem* PlayerEquipment::mainHand() const {
    return main_hand_ ? &*main_hand_ : nullptr;
}

std::int32_t PlayerEquipment::totalWeight(
    const ItemDatabase& database) const {
    if (!main_hand_) {
        return 0;
    }
    const ItemDefinition* definition =
        database.find(
            main_hand_->category,
            main_hand_->definition_id);
    return definition
        ? definition->weight * main_hand_->quantity
        : 0;
}

std::int32_t PlayerEquipment::derivedParameterBonus(
    std::size_t parameter,
    const ItemDatabase& database) const {
    if (!main_hand_ ||
        parameter >= ItemDefinition::derived_parameter_count) {
        return 0;
    }
    const ItemDefinition* definition =
        database.find(
            main_hand_->category,
            main_hand_->definition_id);
    return definition
        ? definition->derived_parameter_bonuses[parameter]
        : 0;
}

}  // namespace osf
