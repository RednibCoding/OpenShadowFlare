#include "items/item_database.hpp"
#include "items/player_inventory.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool checkDefinition(
    const osf::ItemDatabase& database,
    std::int32_t category,
    std::int32_t id,
    const char* name,
    std::int32_t inventory_group,
    std::int32_t inventory_pattern,
    std::int32_t ground_resource,
    std::int32_t ground_chart,
    std::int32_t red_strength,
    std::int32_t green_strength,
    std::int32_t blue_strength) {
    const osf::ItemDefinition* definition =
        database.find(category, id);
    return check(
        definition &&
            definition->name == name &&
            definition->category == category &&
            definition->id == id &&
            definition->inventory_pattern_group ==
                inventory_group &&
            definition->inventory_pattern ==
                inventory_pattern &&
            definition->ground_resource_id ==
                ground_resource &&
            definition->ground_animation_chart ==
                ground_chart &&
            definition->inventory_palette == -1 &&
            definition->ground_red_strength == red_strength &&
            definition->ground_green_strength == green_strength &&
            definition->ground_blue_strength == blue_strength,
        "A known retail item definition differs from Item.Ibn.");
}

bool testRetailDatabase() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path path =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "System" / "Game" /
        "Parameter" / "Item.Ibn";
    if (!std::filesystem::is_regular_file(path)) {
        return true;
    }

    osf::ItemDatabase database;
    std::string error;
    if (!check(
            database.load(path, &error),
            "The retail item database no longer decodes.")) {
        std::cerr << error << '\n';
        return false;
    }
    if (!check(
            database.definitionCount() == 2860 &&
                database.definitions(0).size() == 1264 &&
                database.definitions(1).size() == 1281 &&
                database.definitions(2).size() == 239 &&
                database.definitions(3).size() == 31 &&
                database.definitions(4).size() == 45,
            "The retail item category counts differ.")) {
        return false;
    }
    const osf::ItemDefinition* short_sword =
        database.find(0, 0);
    if (!check(
            short_sword &&
                short_sword->inventory_width == 1 &&
                short_sword->inventory_height == 4 &&
                short_sword->weight == 30,
            "The Short Sword inventory footprint or weight differs.")) {
        return false;
    }
    if (!checkDefinition(
            database,
            0,
            0,
            "Short Sword",
            0,
            0,
            0,
            0,
            1000,
            1000,
            1000) ||
        !checkDefinition(
            database,
            0,
            100,
            "Dagger",
            0,
            279,
            0,
            36,
            1000,
            1000,
            1000) ||
        !checkDefinition(
            database,
            1,
            1000000,
            "Round Shield",
            0,
            45,
            0,
            5,
            900,
            800,
            500) ||
        !checkDefinition(
            database,
            4,
            0,
            "Gold",
            0,
            270,
            0,
            30,
            1000,
            1000,
            1000)) {
        return false;
    }

    std::ifstream input(path, std::ios::binary);
    std::vector<std::uint8_t> damaged{
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()};
    damaged[16] ^= 1u;
    return check(
        !database.decode(damaged),
        "An Item.Ibn payload with a bad checksum was accepted.");
#else
    return true;
#endif
}

bool testPlayerInventory() {
    osf::PlayerInventory inventory;
    if (!check(
            inventory.add(0, 0) &&
                inventory.items().size() == 1 &&
                inventory.items()[0].category == 0 &&
                inventory.items()[0].definition_id == 0 &&
                inventory.items()[0].quantity == 1,
            "An ordinary pickup did not enter player inventory.")) {
        return false;
    }

    inventory.clear();
    if (!check(
            inventory.add(4, 0, 15000) &&
                inventory.items().size() == 2 &&
                inventory.items()[0].quantity == 10000 &&
                inventory.items()[0].grid_x == 0 &&
                inventory.items()[0].grid_y == 0 &&
                inventory.items()[1].quantity == 5000 &&
                inventory.items()[1].grid_x == 1 &&
                inventory.items()[1].grid_y == 0 &&
                inventory.add(4, 0, 5000) &&
                inventory.items().size() == 2 &&
                inventory.items()[1].quantity == 10000 &&
                inventory.gold() == 20000,
            "Retail gold stacks were not filled to 10000.")) {
        return false;
    }

    inventory.clear();
    osf::ItemDefinition sword;
    sword.category = 0;
    sword.id = 0;
    sword.inventory_width = 1;
    sword.inventory_height = 4;
    sword.weight = 30;
    osf::ItemDefinition shield;
    shield.category = 1;
    shield.id = 1000000;
    shield.inventory_width = 2;
    shield.inventory_height = 3;
    shield.weight = 40;
    if (!check(
            inventory.add(sword) &&
                inventory.add(shield) &&
                inventory.items().size() == 2 &&
                inventory.items()[0].grid_x == 0 &&
                inventory.items()[0].grid_y == 0 &&
                inventory.items()[0].width == 1 &&
                inventory.items()[0].height == 4 &&
                inventory.items()[1].grid_x == 1 &&
                inventory.items()[1].grid_y == 0 &&
                inventory.items()[1].width == 2 &&
                inventory.items()[1].height == 3,
            "Retail-sized items were not placed in the 9-by-4 grid.")) {
        return false;
    }
    const std::optional<osf::InventoryItem> held_sword =
        inventory.take(0);
    const osf::InventoryPlacementResult invalid =
        held_sword
            ? inventory.place(*held_sword, 4, 1)
            : osf::InventoryPlacementResult{};
    if (!check(
            held_sword &&
                !invalid.accepted &&
                inventory.items().size() == 1 &&
                inventory.items()[0].category == 1,
            "An invalid owned-item move changed the inventory.")) {
        return false;
    }
    const osf::InventoryPlacementResult displaced =
        inventory.place(*held_sword, 1, 0);
    if (!check(
            displaced.accepted &&
                displaced.held_item &&
                displaced.held_item->category == 1 &&
                inventory.items().size() == 1 &&
                inventory.items()[0].grid_x == 1 &&
                inventory.items()[0].grid_y == 0,
            "Placing onto one item did not leave that item held.")) {
        return false;
    }
    const osf::InventoryPlacementResult placed =
        inventory.place(*displaced.held_item, 4, 0);
    if (!check(
            placed.accepted &&
                !placed.held_item &&
                inventory.items().size() == 2 &&
                inventory.items()[1].grid_x == 4 &&
                inventory.items()[1].grid_y == 0,
            "A displaced item could not be placed in free cells.")) {
        return false;
    }

    inventory.clear();
    for (std::int32_t index = 0;
         index < 36;
         ++index) {
        if (!inventory.add(0, index)) {
            return check(
                false,
                "A free 9-by-4 inventory cell was rejected.");
        }
    }
    return check(
        inventory.items().size() == 36 &&
            !inventory.add(0, 36) &&
            inventory.items().size() == 36,
        "A full inventory accepted another item.");

}

}  // namespace

int main() {
    osf::ItemDatabase database;
    return check(
               !database.decode({}),
               "An empty item database was accepted.") &&
                   testRetailDatabase() &&
                   testPlayerInventory()
               ? 0
               : 1;
}
