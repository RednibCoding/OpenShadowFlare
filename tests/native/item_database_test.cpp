#include "items/item_database.hpp"
#include "items/player_inventory.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
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
            definition->inventory_shadow_pattern == -1 &&
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
                inventory.items()[1].quantity == 5000 &&
                inventory.add(4, 0, 5000) &&
                inventory.items().size() == 2 &&
                inventory.items()[1].quantity == 10000,
            "Retail gold stacks were not filled to 10000.")) {
        return false;
    }

    return true;
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
