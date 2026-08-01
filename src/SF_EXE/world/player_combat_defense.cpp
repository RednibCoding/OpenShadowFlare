#include "player_combat_defense.hpp"
#include "player_element_condition.hpp"

#include "core/retail_integer.hpp"
#include "items/item_database.hpp"
#include "items/item_instance_values.hpp"
#include "items/player_equipment.hpp"
#include "items/player_inventory.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace osf {
namespace {

void addItemStrengths(
    std::array<std::int32_t, 8>& affinities,
    const InventoryItem* item,
    const ItemDatabase& item_database) {
    if (!item) {
        return;
    }
    if (item->category <= 1 &&
        item->durability == 0) {
        return;
    }
    const ItemDefinition* definition =
        item_database.find(
            item->category,
            item->definition_id);
    if (!definition) {
        return;
    }
    for (std::size_t element = 0;
         element < affinities.size();
         ++element) {
        affinities[element] = retailAdd(
            affinities[element],
            retailItemElementStrength(
                *item, *definition, element));
    }
}

std::array<std::int32_t, 8> baseAffinities(
    const PlayerCombatDefenseSnapshot& snapshot) {
    std::array<std::int32_t, 8> affinities{};
    const auto& anchors = retailElementAnchors();
    for (std::size_t element = 0;
         element < affinities.size();
         ++element) {
        const double x =
            static_cast<double>(anchors[element].x) -
            static_cast<double>(snapshot.element_x);
        const double y =
            static_cast<double>(anchors[element].y) -
            static_cast<double>(snapshot.element_y);
        const std::int32_t distance =
            static_cast<std::int32_t>(
                std::trunc(std::hypot(x, y)));
        affinities[element] =
            retailSubtract(20000, distance) / 2000;
    }
    return affinities;
}

}  // namespace

std::array<std::int32_t, 8> buildPlayerElementAffinities(
    const PlayerCombatDefenseSnapshot& snapshot,
    const PlayerEquipment& equipment,
    const PlayerInventory& inventory,
    const ItemDatabase& item_database) {
    std::array<std::int32_t, 8> affinities =
        baseAffinities(snapshot);

    const InventoryItem* main_hand =
        equipment.item(EquipmentSlot::main_hand);
    addItemStrengths(
        affinities, main_hand, item_database);
    addItemStrengths(
        affinities,
        equipment.item(EquipmentSlot::helmet),
        item_database);
    addItemStrengths(
        affinities,
        equipment.item(EquipmentSlot::body),
        item_database);
    addItemStrengths(
        affinities,
        equipment.item(EquipmentSlot::boots),
        item_database);

    const ItemDefinition* main_hand_definition =
        main_hand
            ? item_database.find(
                  main_hand->category,
                  main_hand->definition_id)
            : nullptr;
    if (!main_hand_definition ||
        !main_hand_definition->suppresses_off_hand) {
        addItemStrengths(
            affinities,
            equipment.item(EquipmentSlot::off_hand),
            item_database);
    }

    constexpr std::array<EquipmentSlot, 4> kAccessories = {
        EquipmentSlot::accessory_1,
        EquipmentSlot::accessory_2,
        EquipmentSlot::accessory_3,
        EquipmentSlot::accessory_4,
    };
    for (EquipmentSlot slot : kAccessories) {
        addItemStrengths(
            affinities,
            equipment.item(slot),
            item_database);
    }

    for (const InventoryItem& item : inventory.items()) {
        if (item.category != 2 ||
            item.identified == 0) {
            continue;
        }
        const ItemDefinition* definition =
            item_database.find(
                item.category,
                item.definition_id);
        if (!definition ||
            definition->inventory_width == 1) {
            continue;
        }
        addItemStrengths(
            affinities, &item, item_database);
    }

    for (std::int32_t& affinity : affinities) {
        affinity = std::clamp<std::int32_t>(
            affinity, -10, 10);
    }
    return affinities;
}

CombatDefense buildPlayerCombatDefense(
    const PlayerCombatDefenseSnapshot& snapshot,
    const PlayerEquipment& equipment,
    const PlayerInventory& inventory,
    const ItemDatabase& item_database) {
    CombatDefense defense;
    defense[0] = 0;
    defense[1] = snapshot.character_number;
    defense[2] = snapshot.attack;
    defense[3] = snapshot.physical_defense;
    defense[4] = snapshot.magical_defense;

    const std::array<std::int32_t, 8> affinities =
        buildPlayerElementAffinities(
            snapshot,
            equipment,
            inventory,
            item_database);
    for (std::size_t element = 0;
         element < affinities.size();
         ++element) {
        defense[element + 5] = affinities[element];
    }

    // FUN_00443cb0 leaves the final word unwritten for family-zero
    // player defenses. Its damage paths never consume that word.
    defense[13] = 0;
    return defense;
}

}  // namespace osf
