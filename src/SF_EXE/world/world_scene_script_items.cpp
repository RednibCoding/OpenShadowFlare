#include "world_scene.hpp"

#include "items/item_instance_factory.hpp"

#include <array>
#include <cstdint>
#include <utility>

namespace osf {
namespace {

// Script commands 58 and 59 use this pointer order in retail. The alternate
// weapon set, belt, Warehouse, and Giant Warehouse are deliberately absent.
constexpr std::array<EquipmentSlot, 9> kScriptEquipmentOrder{{
    EquipmentSlot::main_hand,
    EquipmentSlot::body,
    EquipmentSlot::off_hand,
    EquipmentSlot::helmet,
    EquipmentSlot::boots,
    EquipmentSlot::accessory_1,
    EquipmentSlot::accessory_2,
    EquipmentSlot::accessory_3,
    EquipmentSlot::accessory_4,
}};

}  // namespace

bool WorldScene::queryScriptItem(
    std::int32_t category,
    std::int32_t definition_id,
    bool& present) const {
    present =
        player_automatic_items_.contains(category, definition_id) ||
        player_inventory_.contains(category, definition_id);
    if (present) {
        return true;
    }
    for (EquipmentSlot slot : kScriptEquipmentOrder) {
        const InventoryItem* item = player_equipment_.item(slot);
        if (item && item->category == category &&
            item->definition_id == definition_id) {
            present = true;
            break;
        }
    }
    return true;
}

bool WorldScene::removeScriptItem(
    std::int32_t category,
    std::int32_t definition_id) {
    if (player_automatic_items_.removeFirst(
            category, definition_id) ||
        player_inventory_.removeFirst(
            category, definition_id)) {
        return true;
    }
    for (EquipmentSlot slot : kScriptEquipmentOrder) {
        const InventoryItem* item = player_equipment_.item(slot);
        if (!item || item->category != category ||
            item->definition_id != definition_id) {
            continue;
        }
        player_equipment_.take(slot);
        refreshPlayerRuntimeProfile();
        refreshPlayerAppearance();
        return true;
    }
    // Retail treats removing an absent item as a successful no-op.
    return true;
}

bool WorldScene::addScriptItem(
    std::int32_t category,
    std::int32_t definition_id) {
    const ItemDefinition* definition =
        item_database_.find(category, definition_id);
    if (!definition ||
        definition->automatic_inventory_page < 0 ||
        definition->automatic_inventory_page >=
            static_cast<std::int32_t>(
                PlayerAutomaticItems::page_count)) {
        // The retail command also completes when no definition can be made
        // or when the record has no automatic page.
        return true;
    }
    if (player_automatic_items_.contains(
            category, definition_id)) {
        return true;
    }
    InventoryItem item = makeRetailInventoryItem(
        *definition,
        [this]() { return item_random_.next(); });
    player_automatic_items_.add(
        *definition, std::move(item));
    return true;
}

}  // namespace osf
