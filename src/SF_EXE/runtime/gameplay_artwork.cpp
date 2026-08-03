#include "gameplay_artwork.hpp"

#include "items/item_database.hpp"
#include "items/player_belt.hpp"
#include "items/player_equipment.hpp"
#include "items/player_giant_warehouse.hpp"
#include "items/player_inventory.hpp"
#include "items/player_special_items.hpp"
#include "items/vendor_inventory.hpp"
#include "resources/item_inventory_resource.hpp"
#include "resources/resource_manager.hpp"
#include "runtime/gameplay_ui_controller.hpp"
#include "world/world_scene.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace osf::runtime {
namespace {

using ItemGroups = std::array<
    std::uint8_t,
    ItemInventoryResource::group_count>;

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

void requireItemGroup(
    ItemGroups& groups,
    const ItemDatabase& database,
    const InventoryItem& item) {
    const ItemDefinition* definition = database.find(
        item.category, item.definition_id);
    if (!definition ||
        definition->inventory_pattern_group < 0 ||
        static_cast<std::size_t>(
            definition->inventory_pattern_group) >= groups.size()) {
        return;
    }
    groups[static_cast<std::size_t>(
        definition->inventory_pattern_group)] = 1;
}

void requireItemGroups(
    ItemGroups& groups,
    const ItemDatabase& database,
    const std::vector<InventoryItem>& items) {
    for (const InventoryItem& item : items) {
        requireItemGroup(groups, database, item);
    }
}

ItemGroups requiredItemGroups(
    const WorldScene& world,
    const GameplayUiController& ui) {
    ItemGroups groups{};
    const ItemDatabase& database = world.itemDatabase();
    const GameplayInventory& inventory = ui.inventory();

    // Belt contents are part of the always-visible HUD. Backpack and storage
    // sheets only become useful while their corresponding panel is open.
    requireItemGroups(groups, database, world.playerBelt().items());
    if (inventory.active()) {
        requireItemGroups(
            groups, database, world.playerInventory().items());
        for (std::size_t index = 0;
             index < PlayerEquipment::slot_count;
             ++index) {
            const InventoryItem* item = world.playerEquipment().item(
                static_cast<EquipmentSlot>(index));
            if (item) {
                requireItemGroup(groups, database, *item);
            }
        }
    }
    if (inventory.specialItemsActive()) {
        requireItemGroups(
            groups, database, world.playerSpecialItems().items());
    }
    if (inventory.giantWarehouseActive()) {
        const PlayerGiantWarehouse& warehouse =
            world.playerGiantWarehouse();
        requireItemGroups(
            groups,
            database,
            warehouse.page(warehouse.selectedPage()).items());
    }
    if (ui.vendor().active()) {
        const VendorInventory* stock =
            world.vendorInventory(ui.vendor().inventoryIndex());
        if (stock) {
            requireItemGroups(groups, database, stock->items());
        }
    }
    if (const InventoryItem* held = inventory.heldItem()) {
        requireItemGroup(groups, database, *held);
    }
    return groups;
}

}  // namespace

bool synchronizeGameplayArtwork(
    ResourceManager& resources,
    WorldScene& world,
    const GameplayUiController& ui,
    std::string* error) {
    const bool status_required =
        ui.options().active() ||
        ui.blackjack().active() ||
#if OSF_ENABLE_DEBUG_TOOLS
        ui.debug().active() ||
#endif
        ui.equipmentColor().active() ||
        ui.inventory().anyItemPanelActive() ||
        ui.inventory().holdingItem() ||
        ui.map().active() ||
        ui.magic().active() ||
        ui.status().active() ||
        ui.missionList().active() ||
        ui.transport().active() ||
        ui.vendor().active();

    const bool patterns_ready =
        resources.prepareGameplayPattern(
            6,
            "System\\Game\\Pattern\\Status.njp",
            status_required) &&
        resources.prepareGameplayPattern(
            7,
            "System\\Game\\Pattern\\MapIcon.njp",
            ui.map().active()) &&
        resources.prepareGameplayPattern(
            11,
            "System\\Game\\Pattern\\Card.njp",
            ui.blackjack().active());
    if (!patterns_ready) {
        setError(error, "Optional gameplay artwork could not be prepared.");
        return false;
    }

    if (!world.prepareItemInventoryPatterns(
            requiredItemGroups(world, ui), error)) {
        return false;
    }
    if (error) {
        error->clear();
    }
    return true;
}

}  // namespace osf::runtime
