#include "items/item_database.hpp"
#include "items/player_equipment.hpp"
#include "items/player_inventory.hpp"
#include "world/player_combat_defense.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

void writeStateI32(
    std::vector<std::uint8_t>& state,
    std::size_t word,
    std::int32_t value) {
    const std::size_t offset = word * 4u;
    const std::uint32_t raw =
        static_cast<std::uint32_t>(value);
    state[offset] =
        static_cast<std::uint8_t>(raw);
    state[offset + 1] =
        static_cast<std::uint8_t>(raw >> 8u);
    state[offset + 2] =
        static_cast<std::uint8_t>(raw >> 16u);
    state[offset + 3] =
        static_cast<std::uint8_t>(raw >> 24u);
}

osf::InventoryItem itemWithElement(
    const osf::ItemDefinition& definition,
    std::size_t element,
    std::int32_t strength) {
    osf::InventoryItem item =
        osf::makeInventoryItem(definition);
    const std::size_t state_size =
        definition.category == 2 ? 192u : 200u;
    item.retail_state.resize(state_size);
    writeStateI32(
        item.retail_state,
        39u + element,
        strength);
    return item;
}

bool testBaseAffinityGeometry() {
    osf::ItemDatabase database;
    osf::PlayerEquipment equipment;
    osf::PlayerInventory inventory;
    osf::PlayerCombatDefenseSnapshot snapshot;

    if (!check(
            osf::buildPlayerElementAffinities(
                snapshot,
                equipment,
                inventory,
                database) ==
                std::array<std::int32_t, 8>{
                    0, 0, 0, 0, 0, 0, 0, 0},
            "The neutral retail element position differs.")) {
        return false;
    }

    snapshot.character_number = 7;
    snapshot.attack = 101;
    snapshot.physical_defense = 202;
    snapshot.magical_defense = 303;
    snapshot.element_y = 20000;
    const std::array<std::int32_t, 8> expected = {
        10, -10, -4, -4, -8, 2, -8, 2};
    const osf::CombatDefense defense =
        osf::buildPlayerCombatDefense(
            snapshot,
            equipment,
            inventory,
            database);
    if (!check(
            osf::buildPlayerElementAffinities(
                snapshot,
                equipment,
                inventory,
                database) == expected,
            "The retail element-anchor geometry differs.") ||
        !check(
            defense[0] == 0 &&
                defense[1] == 7 &&
                defense[2] == 101 &&
                defense[3] == 202 &&
                defense[4] == 303 &&
                defense[5] == 10 &&
                defense[12] == 2 &&
                defense[13] == 0,
            "The player combat-defense word layout differs.")) {
        return false;
    }
    return true;
}

bool testRetailItemContributors() {
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
            "The retail item database could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }

    const osf::ItemDefinition* glass_sword =
        database.find(0, 1600);
    const osf::ItemDefinition* glass_shield =
        database.find(1, 1010100);
    const osf::ItemDefinition* battle_axe =
        database.find(0, 4000000);
    const osf::ItemDefinition* gold_giant_sword =
        database.find(0, 80000030);
    const osf::ItemDefinition* stone =
        database.find(2, 1000000);
    const osf::ItemDefinition* monolis =
        database.find(2, 30000000);
    if (!check(
            glass_sword &&
                glass_sword->element_strengths[1] == 3 &&
                glass_shield &&
                glass_shield->element_strengths[1] == 2 &&
                battle_axe &&
                battle_axe->suppresses_off_hand &&
                gold_giant_sword &&
                !gold_giant_sword->suppresses_off_hand &&
                stone &&
                stone->inventory_width == 1 &&
                monolis &&
                monolis->inventory_width != 1,
            "Known retail element-contributor definitions differ.")) {
        return false;
    }

    osf::PlayerCombatDefenseSnapshot snapshot;
    osf::PlayerEquipment equipment;
    osf::PlayerInventory inventory;
    if (!check(
            equipment.place(
                osf::EquipmentSlot::main_hand,
                itemWithElement(*glass_sword, 1, 0),
                *glass_sword,
                999).accepted &&
                equipment.place(
                    osf::EquipmentSlot::off_hand,
                    itemWithElement(*glass_shield, 1, 0),
                    *glass_shield,
                    999).accepted &&
                equipment.place(
                    osf::EquipmentSlot::accessory_1,
                    itemWithElement(*stone, 1, 1),
                    *stone,
                    999).accepted,
            "The equipped element fixtures could not be placed.")) {
        return false;
    }
    osf::InventoryItem passive =
        itemWithElement(*monolis, 1, 4);
    passive.identified = 1;
    if (!check(
            inventory.place(
                passive, 0, 0).accepted,
            "The passive backpack fixture could not be placed.") ||
        !check(
            osf::buildPlayerElementAffinities(
                snapshot,
                equipment,
                inventory,
                database)[1] == 10,
            "Equipped and identified backpack elements were not summed "
            "and clamped like retail.")) {
        return false;
    }

    osf::PlayerInventory unidentified_inventory;
    passive.identified = 0;
    if (!check(
            unidentified_inventory.place(
                passive, 0, 0).accepted,
            "The unidentified backpack fixture could not be placed.") ||
        !check(
            osf::buildPlayerElementAffinities(
                snapshot,
                equipment,
                unidentified_inventory,
                database)[1] == 6,
            "An unidentified backpack item contributed an element.")) {
        return false;
    }

    osf::PlayerInventory one_cell_inventory;
    osf::InventoryItem carried_accessory =
        itemWithElement(*stone, 1, 9);
    carried_accessory.identified = 1;
    if (!check(
            one_cell_inventory.place(
                carried_accessory, 0, 0).accepted,
            "The carried accessory fixture could not be placed.") ||
        !check(
            osf::buildPlayerElementAffinities(
                snapshot,
                osf::PlayerEquipment{},
                one_cell_inventory,
                database)[1] == 0,
            "A one-cell backpack accessory contributed an element.")) {
        return false;
    }

    osf::PlayerEquipment negative_accessory;
    if (!check(
            negative_accessory.place(
                osf::EquipmentSlot::accessory_1,
                itemWithElement(*stone, 0, -20),
                *stone,
                999).accepted &&
                osf::buildPlayerElementAffinities(
                    snapshot,
                    negative_accessory,
                    osf::PlayerInventory{},
                    database)[0] == -10,
            "A negative item element did not use the retail lower clamp.")) {
        return false;
    }

    osf::PlayerEquipment two_handed;
    if (!check(
            two_handed.place(
                osf::EquipmentSlot::main_hand,
                itemWithElement(*battle_axe, 1, 0),
                *battle_axe,
                999).accepted &&
                two_handed.place(
                    osf::EquipmentSlot::off_hand,
                    itemWithElement(*glass_shield, 1, 2),
                    *glass_shield,
                    999).accepted &&
                osf::buildPlayerElementAffinities(
                    snapshot,
                    two_handed,
                    osf::PlayerInventory{},
                    database)[1] ==
                    std::clamp<std::int32_t>(
                        battle_axe->element_strengths[1],
                        -10,
                        10),
            "A retail two-handed weapon did not suppress off-hand "
            "element values.")) {
        return false;
    }

    osf::PlayerEquipment unique_weapon;
    if (!check(
            unique_weapon.place(
                osf::EquipmentSlot::main_hand,
                itemWithElement(*gold_giant_sword, 1, 0),
                *gold_giant_sword,
                999).accepted &&
                unique_weapon.place(
                    osf::EquipmentSlot::off_hand,
                    itemWithElement(*glass_shield, 1, 2),
                    *glass_shield,
                    999).accepted &&
                osf::buildPlayerElementAffinities(
                    snapshot,
                    unique_weapon,
                    osf::PlayerInventory{},
                    database)[1] ==
                    std::clamp<std::int32_t>(
                        gold_giant_sword->element_strengths[1] +
                            glass_shield->element_strengths[1] +
                            2,
                        -10,
                        10),
            "An unrelated unique-weapon field suppressed the off hand.")) {
        return false;
    }
#endif
    return true;
}

bool testIdentificationDefaults() {
    osf::ItemDefinition normal;
    normal.category = 0;
    normal.id = 1;
    normal.variant = 0;
    osf::ItemDefinition unidentified = normal;
    unidentified.id = 2;
    unidentified.variant = 1;
    osf::ItemDefinition unidentified_two = normal;
    unidentified_two.id = 3;
    unidentified_two.variant = 2;
    osf::ItemDefinition known_unique = normal;
    known_unique.id = 4;
    known_unique.variant = 3;

    osf::PlayerInventory inventory;
    return check(
        osf::makeInventoryItem(normal).identified == 1 &&
            osf::makeInventoryItem(unidentified).identified == 0 &&
            osf::makeInventoryItem(unidentified_two).identified == 0 &&
            osf::makeInventoryItem(known_unique).identified == 1 &&
            inventory.add(normal) &&
            inventory.items().front().identified == 1,
        "New item identification does not follow the retail variant rule.");
}

}  // namespace

int main() {
    return testBaseAffinityGeometry() &&
                   testRetailItemContributors() &&
                   testIdentificationDefaults()
        ? 0
        : 1;
}
