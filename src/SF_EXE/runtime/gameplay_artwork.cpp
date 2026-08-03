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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace osf::runtime {
namespace {

constexpr std::size_t kStatusPatternCount = 121;
using StatusPatterns = std::vector<std::uint8_t>;
using ItemPatterns = ItemInventoryResource::PatternSelection;

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

void enablePattern(
    StatusPatterns& patterns,
    std::size_t pattern) {
    if (pattern < patterns.size()) {
        patterns[pattern] = 1;
    }
}

void enablePatterns(
    StatusPatterns& patterns,
    std::initializer_list<std::size_t> requested) {
    for (const std::size_t pattern : requested) {
        enablePattern(patterns, pattern);
    }
}

void enablePatternRange(
    StatusPatterns& patterns,
    std::size_t first,
    std::size_t last) {
    for (std::size_t pattern = first;
         pattern <= last;
         ++pattern) {
        enablePattern(patterns, pattern);
    }
}

StatusPatterns requiredStatusPatterns(
    const WorldScene& world,
    const GameplayUiController& ui) {
    StatusPatterns patterns(kStatusPatternCount, 0);
    const GameplayInventory& inventory = ui.inventory();

    if (ui.status().active()) {
        enablePattern(patterns, 5);
        enablePatternRange(patterns, 36, 57);
    }
    if (ui.magic().active()) {
        enablePatterns(patterns, {6, 32, 69, 70});
    }
    if (inventory.active()) {
        enablePatterns(
            patterns,
            {
                world.playerData().gender() == 1 ? 0u : 1u,
                2, 3, 16, 67, 74, 75,
            });
    }
    if (inventory.specialItemsActive()) {
        enablePatterns(patterns, {14, 15, 16});
    }
    if (inventory.giantWarehouseActive()) {
        enablePatterns(patterns, {14, 15, 16, 73});
        enablePatternRange(patterns, 74, 94);
    }
    if (inventory.holdingItem()) {
        enablePattern(patterns, 16);
    }
    if (ui.vendor().active()) {
        enablePatterns(patterns, {7, 8, 9, 16});
    }
    if (ui.transport().active()) {
        enablePatterns(patterns, {11, 12, 13, 22, 23});
    }
    if (ui.map().active()) {
        enablePatterns(patterns, {71, 118});
    }
    if (ui.missionList().active()) {
        enablePatterns(
            patterns,
            {10, 25, 26, 58, 59, 110, 111, 112, 113});
    }
    if (ui.equipmentColor().active()) {
        enablePatternRange(patterns, 102, 109);
    }
    if (ui.blackjack().active()) {
        enablePattern(patterns, 119);
    }
#if OSF_ENABLE_DEBUG_TOOLS
    if (ui.debug().active()) {
        enablePatterns(patterns, {58, 59});
    }
#endif
    if (ui.options().active()) {
        if (ui.options().page() == GameplayOptionsPage::help) {
            enablePatterns(patterns, {10, 27, 28, 29, 30, 66});
        } else {
            enablePatterns(patterns, {58, 59});
            if (ui.options().page() == GameplayOptionsPage::settings) {
                enablePatterns(patterns, {68, 120});
            }
        }
    }
    return patterns;
}

bool anyPatternRequired(const StatusPatterns& patterns) {
    return std::any_of(
        patterns.begin(),
        patterns.end(),
        [](std::uint8_t enabled) { return enabled != 0; });
}

void requireItemPattern(
    ItemPatterns& patterns,
    const ItemDatabase& database,
    const InventoryItem& item) {
    const ItemDefinition* definition = database.find(
        item.category, item.definition_id);
    if (!definition ||
        definition->inventory_pattern_group < 0 ||
        static_cast<std::size_t>(
            definition->inventory_pattern_group) >= patterns.size() ||
        definition->inventory_pattern < 0) {
        return;
    }
    std::vector<std::uint8_t>& group =
        patterns[static_cast<std::size_t>(
            definition->inventory_pattern_group)];
    const std::size_t pattern = static_cast<std::size_t>(
        definition->inventory_pattern);
    if (group.size() <= pattern) {
        group.resize(pattern + 1, 0);
    }
    group[pattern] = 1;
}

void requireItemPatterns(
    ItemPatterns& patterns,
    const ItemDatabase& database,
    const std::vector<InventoryItem>& items) {
    for (const InventoryItem& item : items) {
        requireItemPattern(patterns, database, item);
    }
}

ItemPatterns requiredItemPatterns(
    const WorldScene& world,
    const GameplayUiController& ui) {
    ItemPatterns patterns;
    const ItemDatabase& database = world.itemDatabase();
    const GameplayInventory& inventory = ui.inventory();

    // Belt contents are part of the always-visible HUD. Backpack and storage
    // sheets only become useful while their corresponding panel is open.
    requireItemPatterns(
        patterns, database, world.playerBelt().items());
    if (inventory.active()) {
        requireItemPatterns(
            patterns, database, world.playerInventory().items());
        for (std::size_t index = 0;
             index < PlayerEquipment::slot_count;
             ++index) {
            const InventoryItem* item = world.playerEquipment().item(
                static_cast<EquipmentSlot>(index));
            if (item) {
                requireItemPattern(patterns, database, *item);
            }
        }
    }
    if (inventory.specialItemsActive()) {
        requireItemPatterns(
            patterns, database, world.playerSpecialItems().items());
    }
    if (inventory.giantWarehouseActive()) {
        const PlayerGiantWarehouse& warehouse =
            world.playerGiantWarehouse();
        requireItemPatterns(
            patterns,
            database,
            warehouse.page(warehouse.selectedPage()).items());
    }
    if (ui.vendor().active()) {
        const VendorInventory* stock =
            world.vendorInventory(ui.vendor().inventoryIndex());
        if (stock) {
            requireItemPatterns(patterns, database, stock->items());
        }
    }
    if (const InventoryItem* held = inventory.heldItem()) {
        requireItemPattern(patterns, database, *held);
    }
    return patterns;
}

}  // namespace

bool synchronizeGameplayArtwork(
    ResourceManager& resources,
    WorldScene& world,
    const GameplayUiController& ui,
    std::string* error) {
    const StatusPatterns status_patterns =
        requiredStatusPatterns(world, ui);
    const bool status_required =
        anyPatternRequired(status_patterns);

    const bool patterns_ready =
        resources.prepareGameplayPattern(
            6,
            "System\\Game\\Pattern\\Status.njp",
            status_patterns,
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
            requiredItemPatterns(world, ui), error)) {
        return false;
    }
    if (error) {
        error->clear();
    }
    return true;
}

}  // namespace osf::runtime
