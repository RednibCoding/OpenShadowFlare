#include "items/item_database.hpp"
#include "items/item_condition.hpp"
#include "items/player_belt.hpp"
#include "items/player_equipment.hpp"
#include "items/player_inventory.hpp"
#include "items/player_special_items.hpp"

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
    const osf::ItemDefinition* leather_cloth =
        database.find(1, 0);
    const osf::ItemDefinition* round_shield =
        database.find(1, 1000000);
    const osf::ItemDefinition* leather_hat =
        database.find(1, 2000000);
    const osf::ItemDefinition* boots =
        database.find(1, 3000000);
    const osf::ItemDefinition* dagger =
        database.find(0, 100);
    const osf::ItemDefinition* stone_accessory =
        database.find(2, 1000000);
    const osf::ItemDefinition* blade_ring =
        database.find(2, 4000100);
    const osf::ItemDefinition* tablet =
        database.find(3, 0);
    const osf::ItemDefinition* capsule =
        database.find(3, 10000000);
    const osf::ItemDefinition* white_medicine =
        database.find(3, 30000000);
    if (!check(
            short_sword &&
                short_sword->base_price == 400 &&
                short_sword->maximum_durability == 300 &&
                short_sword->inventory_width == 1 &&
                short_sword->inventory_height == 4 &&
                short_sword->weight == 30 &&
                short_sword->required_level == 1 &&
                short_sword->derived_parameter_bonuses[0] == 20 &&
                short_sword->derived_parameter_bonuses[1] == 100 &&
                short_sword->appearance_part == 12 &&
                short_sword->appearance_red_strength == 1000 &&
                short_sword->appearance_green_strength == 1000 &&
                short_sword->appearance_blue_strength == 1000 &&
                short_sword->secondary_appearance_part == -1 &&
                !short_sword->suppresses_off_hand_appearance &&
            dagger &&
                dagger->base_price == 400 &&
                dagger->maximum_durability == 300 &&
                dagger->derived_parameter_bonuses[0] == 10 &&
                dagger->derived_parameter_bonuses[1] == 120 &&
                dagger->derived_parameter_bonuses[8] == 50 &&
                dagger->element_strengths[0] == 0 &&
                dagger->element_strengths[7] == 0 &&
            stone_accessory &&
                stone_accessory->inventory_width == 1 &&
                stone_accessory->inventory_height == 1 &&
                stone_accessory->required_level == 1 &&
            blade_ring &&
                blade_ring->required_level == 23 &&
            tablet &&
            capsule &&
            white_medicine &&
                white_medicine->inventory_width == 1 &&
                white_medicine->inventory_height == 2,
            "The Short Sword equipment fields differ.")) {
        return false;
    }
    if (!check(
            leather_cloth &&
                leather_cloth->subtype == 1 &&
                leather_cloth->variant == 0 &&
                leather_cloth->required_level == 1 &&
                leather_cloth->appearance_part == 5 &&
                leather_cloth->derived_parameter_bonuses[2] == 20 &&
            round_shield &&
                round_shield->subtype == 2 &&
                round_shield->required_level == 1 &&
                round_shield->appearance_part == 9 &&
                round_shield->appearance_red_strength == 900 &&
                round_shield->appearance_green_strength == 800 &&
                round_shield->appearance_blue_strength == 500 &&
                round_shield->derived_parameter_bonuses[2] == 8 &&
            leather_hat &&
                leather_hat->subtype == 0 &&
                leather_hat->appearance_part == 5 &&
            boots &&
                boots->subtype == 3 &&
                boots->appearance_part == 5,
            "Known armor equipment fields differ from Item.Ibn.")) {
        return false;
    }
    osf::InventoryItem sword_item;
    sword_item.category = short_sword->category;
    sword_item.definition_id = short_sword->id;
    sword_item.width = short_sword->inventory_width;
    sword_item.height = short_sword->inventory_height;
    osf::PlayerEquipment equipment;
    if (!check(
            !equipment.place(
                 osf::EquipmentSlot::main_hand,
                 sword_item, *short_sword, 0).accepted &&
                equipment.place(
                    osf::EquipmentSlot::main_hand,
                    sword_item, *short_sword, 1).accepted &&
                equipment.item(
                    osf::EquipmentSlot::main_hand),
            "The Short Sword level requirement was not enforced.")) {
        return false;
    }
    const auto makeItem = [](const osf::ItemDefinition& definition) {
        osf::InventoryItem item;
        item.category = definition.category;
        item.definition_id = definition.id;
        item.width = definition.inventory_width;
        item.height = definition.inventory_height;
        return item;
    };
    if (!check(
            !equipment.place(
                 osf::EquipmentSlot::helmet,
                 makeItem(*round_shield),
                 *round_shield,
                 1).accepted &&
                equipment.place(
                    osf::EquipmentSlot::helmet,
                    makeItem(*leather_hat),
                    *leather_hat,
                    1).accepted &&
                equipment.place(
                    osf::EquipmentSlot::body,
                    makeItem(*leather_cloth),
                    *leather_cloth,
                    1).accepted &&
                equipment.place(
                    osf::EquipmentSlot::boots,
                    makeItem(*boots),
                    *boots,
                    1).accepted &&
                equipment.place(
                    osf::EquipmentSlot::off_hand,
                    makeItem(*round_shield),
                    *round_shield,
                    1).accepted &&
                equipment.place(
                    osf::EquipmentSlot::accessory_1,
                    makeItem(*stone_accessory),
                    *stone_accessory,
                    1).accepted &&
                !equipment.place(
                    osf::EquipmentSlot::accessory_2,
                    makeItem(*blade_ring),
                    *blade_ring,
                    22).accepted &&
                equipment.place(
                    osf::EquipmentSlot::accessory_2,
                    makeItem(*blade_ring),
                    *blade_ring,
                    23).accepted &&
                equipment.totalWeight(database) == 186 &&
                equipment.derivedParameterBonus(2, database) == 39,
            "The ordinary or accessory equipment slots differ.")) {
        return false;
    }

    osf::PlayerBelt belt;
    const osf::InventoryItem tablet_item =
        makeItem(*tablet);
    const osf::InventoryItem capsule_item =
        makeItem(*capsule);
    const osf::InventoryItem medicine_item =
        makeItem(*white_medicine);
    if (!check(
            belt.place(
                tablet_item, 0, 0, *tablet).accepted &&
                belt.place(
                    capsule_item, 0, 0, *capsule)
                    .held_item->definition_id == tablet->id &&
                belt.place(
                    medicine_item, 3, 0, *white_medicine)
                    .accepted &&
                !belt.place(
                    medicine_item, 2, 1, *white_medicine)
                    .accepted &&
                belt.itemAt(3, 1) &&
                belt.takeAt(3, 1)->definition_id ==
                    white_medicine->id &&
                !belt.place(
                    makeItem(*short_sword),
                    1,
                    0,
                    *short_sword).accepted,
            "The retail 4-by-2 category-three belt rules differ.")) {
        return false;
    }

    osf::PlayerSpecialItems special_items;
    osf::InventoryItem first_gold;
    first_gold.category = 4;
    first_gold.definition_id = 0;
    first_gold.quantity = 9000;
    osf::InventoryItem second_gold = first_gold;
    second_gold.quantity = 2000;
    if (!check(
            special_items.place(first_gold, 0, 0).accepted,
            "The special-item gold fixture could not be placed.")) {
        return false;
    }
    const osf::InventoryPlacementResult merged_gold =
        special_items.place(second_gold, 0, 0);
    osf::InventoryItem invalid_item = tablet_item;
    invalid_item.width = 0;
    if (!check(
            merged_gold.accepted &&
                merged_gold.held_item &&
                merged_gold.held_item->quantity == 1000 &&
                special_items.items().size() == 1 &&
                special_items.items()[0].quantity ==
                    osf::PlayerInventory::maximum_gold_stack &&
                !special_items.place(
                    invalid_item, 1, 0).accepted,
            "The retail special-item grid or gold stacking differs.")) {
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
                inventory.items()[0].durability == 0 &&
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

bool testItemCondition() {
    osf::ItemDefinition definition;
    definition.category = 0;
    definition.id = 100;
    definition.maximum_durability = 300;

    osf::InventoryItem item;
    item.category = definition.category;
    item.definition_id = definition.id;
    if (!check(
            osf::itemCurrentDurability(
                item, definition) == 300 &&
                !osf::itemConditionWarningVisible(
                    item, definition, 0),
            "An undamaged item displayed a condition warning.")) {
        return false;
    }

    item.durability = 30;
    if (!check(
            !osf::itemConditionWarningVisible(
                item, definition, 0),
            "The retail ten-percent boundary displayed too early.")) {
        return false;
    }

    item.durability = 29;
    if (!check(
            osf::itemConditionWarningVisible(
                item, definition, 7) &&
                !osf::itemConditionWarningVisible(
                    item, definition, 8) &&
                !osf::itemConditionWarningVisible(
                    item, definition, 15) &&
                osf::itemConditionWarningVisible(
                    item, definition, 16),
            "Low durability did not use the retail eight-on, "
            "eight-off warning cycle.")) {
        return false;
    }

    item.durability = 0;
    if (!check(
            osf::itemConditionWarningVisible(
                item, definition, 8) &&
                osf::itemConditionWarningVisible(
                    item, definition, 15),
            "A broken item did not keep its condition warning visible.")) {
        return false;
    }

    definition.category = 4;
    item.category = 4;
    definition.maximum_durability = 300;
    return check(
        !osf::itemConditionWarningVisible(
            item, definition, 0),
        "A non-equipment item displayed a durability warning.");
}

}  // namespace

int main() {
    osf::ItemDatabase database;
    return check(
               !database.decode({}),
               "An empty item database was accepted.") &&
                   testRetailDatabase() &&
                   testPlayerInventory() &&
                   testItemCondition()
               ? 0
               : 1;
}
