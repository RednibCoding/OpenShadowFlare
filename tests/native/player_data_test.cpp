#include "items/item_appearance.hpp"
#include "items/item_database.hpp"
#include "items/item_instance_factory.hpp"
#include "items/item_repair.hpp"
#include "items/player_belt.hpp"
#include "items/player_automatic_items.hpp"
#include "items/player_equipment.hpp"
#include "items/player_giant_warehouse.hpp"
#include "items/player_inventory.hpp"
#include "items/player_special_items.hpp"
#include "libs/RKC_DIB/rkc_dib.hpp"
#include "world/player_data.hpp"
#include "world/player_item_controller.hpp"
#include "world/player_magic.hpp"
#include "world/retail_save_file.hpp"
#include "world/retail_save_automatic_items.hpp"
#include "world/retail_save_companion_progress.hpp"
#include "world/retail_save_extension.hpp"
#include "world/retail_save_giant_warehouse.hpp"
#include "world/retail_save_items.hpp"
#include "world/retail_save_magic.hpp"
#include "world/retail_save_mines.hpp"
#include "world/retail_save_preview.hpp"
#include "world/retail_save_progress.hpp"
#include "world/retail_save_world_state.hpp"
#include "world/transport_catalog.hpp"
#include "core/retail_random.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

void writeI32(
    std::array<std::uint8_t, osf::PlayerData::retail_record_size>&
        record,
    std::size_t offset,
    std::int32_t value) {
    const std::uint32_t data = static_cast<std::uint32_t>(value);
    record[offset] = static_cast<std::uint8_t>(data);
    record[offset + 1] = static_cast<std::uint8_t>(data >> 8u);
    record[offset + 2] = static_cast<std::uint8_t>(data >> 16u);
    record[offset + 3] = static_cast<std::uint8_t>(data >> 24u);
}

std::uint32_t readU32(
    const std::vector<std::uint8_t>& bytes,
    std::size_t offset) {
    return
        static_cast<std::uint32_t>(bytes[offset]) |
        (static_cast<std::uint32_t>(bytes[offset + 1]) << 8u) |
        (static_cast<std::uint32_t>(bytes[offset + 2]) << 16u) |
        (static_cast<std::uint32_t>(bytes[offset + 3]) << 24u);
}

void writeU32(
    std::vector<std::uint8_t>& bytes,
    std::size_t offset,
    std::uint32_t value) {
    bytes[offset] = static_cast<std::uint8_t>(value);
    bytes[offset + 1] =
        static_cast<std::uint8_t>(value >> 8u);
    bytes[offset + 2] =
        static_cast<std::uint8_t>(value >> 16u);
    bytes[offset + 3] =
        static_cast<std::uint8_t>(value >> 24u);
}

}  // namespace

int main() {
    osf::TableDatabase tables;
    std::string error;
    if (!check(
            tables.load(
                std::string(OPENSHADOWFLARE_SOURCE_DIR) +
                    "/tmp/ShadowFlare/System/Game/Parameter/Table.Tbd",
                &error),
            "The player parameter tables could not be loaded.")) {
        std::cerr << error << '\n';
        return 1;
    }

    osf::PlayerData male;
    if (!check(
            male.initializeNew(
                "Mina",
                osf::playerGenderValue(osf::PlayerGender::male),
                tables,
                &error),
            "A new male character could not be initialized.")) {
        std::cerr << error << '\n';
        return 1;
    }
    if (!check(
            male.valid() &&
                male.name() == "Mina" &&
                male.gender() == 1 &&
                male.job() == 0x10 &&
                male.level() == 1 &&
                male.baseMaximumLife() == 150 &&
                male.currentLife() == 150 &&
                male.baseMaximumMana() == 150 &&
                male.currentMana() == 150 &&
                male.baseMagicalEvasionRate() ==
                    male.initialParameter(12) &&
                male.walkingSpeedTier() == 5,
            "The male record does not match table 900 and FUN_00440f70.")) {
        return 1;
    }
    osf::PlayerData companion_player = male;
    const osf::TableData* companion_catalog = tables.find(60);
    const std::size_t companion_count =
        companion_catalog
            ? static_cast<std::size_t>(
                  companion_catalog->rowCount())
            : 0u;
    companion_player.setCompanionRespawnCounter(600);
    if (!check(
            companion_player.switchCompanion(0) &&
                companion_player.companionRespawnCounter() == 0,
            "Selecting the already owned companion did not clear its "
            "retail defeated countdown.")) {
        return 1;
    }
    companion_player.awardCompanionKillExperience(
        16000000, 0, true);
    if (!check(
            companion_count != 0 &&
                companion_player.companionCount() ==
                    companion_count &&
                std::all_of(
                    companion_player.companionLevels().begin(),
                    companion_player.companionLevels().end(),
                    [](std::int32_t level) {
                        return level == 1;
                    }) &&
                companion_player.companionExperience(0) == 1 &&
                companion_player.switchCompanion(1) &&
                companion_player.companionType() == 1 &&
                companion_player.companionLevel() == 1 &&
                companion_player.companionExperience() == 0 &&
                companion_player.companionRespawnCounter() == 0,
            "A companion switch did not preserve the old dog and restore "
            "the new dog's retail progression.")) {
        return 1;
    }
    companion_player.awardCompanionKillExperience(
        16000000, 0, true);
    companion_player.awardCompanionKillExperience(
        16000000, 0, true);
    if (!check(
            companion_player.switchCompanion(0) &&
                companion_player.companionExperience() == 1 &&
                companion_player.companionExperience(1) == 2 &&
                !companion_player.switchCompanion(-1) &&
                !companion_player.switchCompanion(
                    static_cast<std::int32_t>(companion_count)),
            "Per-companion experience leaked across a swap or an invalid "
            "catalog row was accepted.")) {
        return 1;
    }
    std::vector<std::uint8_t> companion_payload;
    std::size_t companion_progress_end = 0;
    std::size_t companion_mine_end = 0;
    if (!check(
            osf::replaceRetailCompanionProgress(
                companion_payload,
                0,
                companion_player,
                &companion_progress_end,
                &error) &&
                osf::replaceRetailMineCount(
                    companion_payload,
                    companion_progress_end,
                    7,
                    &companion_mine_end,
                    &error),
            "The portable save could not serialize companion progression "
            "before its retail Mine field.")) {
        std::cerr << error << '\n';
        return 1;
    }
    osf::PlayerData restored_companion_player = male;
    std::int32_t restored_companion_mines = 0;
    std::size_t restored_companion_end = 0;
    std::size_t restored_companion_mine_end = 0;
    if (!check(
            osf::restoreRetailCompanionProgress(
                companion_payload,
                0,
                restored_companion_player,
                &restored_companion_end,
                &error) &&
                osf::restoreRetailMineCount(
                    companion_payload,
                    restored_companion_end,
                    restored_companion_mines,
                    &restored_companion_mine_end,
                    &error) &&
                restored_companion_end == companion_progress_end &&
                restored_companion_mine_end == companion_mine_end &&
                restored_companion_mines == 7 &&
                restored_companion_player.companionExperience(0) == 1 &&
                restored_companion_player.companionExperience(1) == 2 &&
                restored_companion_player.switchCompanion(1) &&
                restored_companion_player.companionExperience() == 2,
            "Distinct companion progression did not survive the retail "
            "save stream beside the Mine count.")) {
        std::cerr << error << '\n';
        return 1;
    }
    osf::PlayerData job_player = male;
    job_player.setJob(osf::PlayerJob::warrior);
    if (!check(
            osf::retailScriptJobSelection(
                osf::playerJobValue(
                    osf::PlayerJob::mercenary)) == 0 &&
                osf::retailScriptJobSelection(
                    osf::playerJobValue(
                        osf::PlayerJob::warrior)) == 1 &&
                osf::retailScriptJobSelection(
                    osf::playerJobValue(
                        osf::PlayerJob::hunter)) == 2 &&
                osf::retailScriptJobSelection(
                    osf::playerJobValue(
                        osf::PlayerJob::spellcaster)) == 3 &&
                job_player.job() ==
                    osf::playerJobValue(
                        osf::PlayerJob::warrior) &&
                osf::retailJobForScriptSelection(1) ==
                    osf::PlayerJob::warrior &&
                osf::retailJobForScriptSelection(2) ==
                    osf::PlayerJob::hunter &&
                osf::retailJobForScriptSelection(3) ==
                    osf::PlayerJob::spellcaster &&
                !osf::retailJobForScriptSelection(0) &&
                !osf::retailJobForScriptSelection(4),
            "The retail script job mapping or saved job mutation differs.")) {
        return 1;
    }

    osf::ItemDatabase items;
    if (!check(
            items.load(
                std::string(OPENSHADOWFLARE_SOURCE_DIR) +
                    "/tmp/ShadowFlare/System/Game/Parameter/Item.Ibn",
                &error),
            "The item data for belt-use testing could not be loaded.")) {
        std::cerr << error << '\n';
        return 1;
    }
    const osf::ItemDefinition* tablet =
        items.find(3, 0);
    const osf::ItemDefinition* capsule =
        items.find(3, 10000000);
    const osf::ItemDefinition* white_medicine =
        items.find(3, 30000000);
    const osf::ItemDefinition* fire_medicine =
        items.find(3, 30000001);
    osf::PlayerBelt belt;
    osf::PlayerItemController item_controller;
    const auto item_use_targets = [](osf::PlayerData& player) {
        return osf::PlayerItemUseTargets{
            player,
            player.baseMaximumLife(),
            player.baseMaximumMana(),
            0,
            0,
            nullptr,
        };
    };
    item_controller.initializeNew();
    if (!check(
            tablet &&
                capsule &&
                white_medicine &&
                fire_medicine &&
                belt.place(
                    osf::makeInventoryItem(*tablet),
                    0,
                    0,
                    *tablet)
                    .accepted &&
                belt.place(
                    osf::makeInventoryItem(*capsule),
                    0,
                    1,
                    *capsule)
                    .accepted &&
                item_controller.mineCount() == 5,
            "The belt-use fixture or initial mine count differs.")) {
        return 1;
    }

    osf::PlayerData belt_player = male;
    if (!check(
            !item_controller
                 .useBeltPocket(
                     0,
                     belt,
                     items,
                     item_use_targets(belt_player))
                 .consumed &&
                belt.itemAt(0, 0),
            "A Tablet was consumed while life was already full.")) {
        return 1;
    }
    belt_player.setCurrentLife(0);
    belt_player.setCurrentMana(0);
    if (!check(
            belt_player.restoreLife(0, 10) &&
                belt_player.currentLife() == 15 &&
                belt_player.restoreMana(0, 10) &&
                belt_player.currentMana() == 15,
            "Percentage life or mana restoration differs.")) {
        return 1;
    }
    belt_player.setCurrentLife(
        belt_player.baseMaximumLife() - 10);
    const osf::PlayerItemUseResult tablet_use =
        item_controller.useBeltPocket(
            0,
            belt,
            items,
            item_use_targets(belt_player));
    belt_player.setCurrentMana(
        belt_player.baseMaximumMana() - 10);
    const osf::PlayerItemUseResult capsule_use =
        item_controller.useBeltPocket(
            4,
            belt,
            items,
            item_use_targets(belt_player));
    if (!check(
            tablet_use.consumed &&
                tablet_use.sound_sample == 16 &&
                !belt.itemAt(0, 0) &&
                belt_player.currentLife() ==
                    belt_player.baseMaximumLife() &&
                capsule_use.consumed &&
                capsule_use.sound_sample == 16 &&
                !belt.itemAt(0, 1) &&
                belt_player.currentMana() ==
                    belt_player.baseMaximumMana(),
            "Tablet/Capsule use or the 1-8 belt mapping differs.")) {
        return 1;
    }

    osf::PlayerInventory medicine_inventory;
    if (!check(
            medicine_inventory.add(*tablet) &&
                medicine_inventory.add(*capsule),
            "The backpack medicine fixture could not be prepared.")) {
        return 1;
    }
    osf::PlayerData inventory_player = male;
    if (!check(
            !item_controller
                 .useInventoryItem(
                     0,
                     medicine_inventory,
                     items,
                     item_use_targets(inventory_player))
                 .consumed &&
                medicine_inventory.items().size() == 2,
            "A backpack Tablet was consumed while life was full.")) {
        return 1;
    }
    inventory_player.setCurrentLife(
        inventory_player.baseMaximumLife() - 10);
    const osf::PlayerItemUseResult inventory_tablet_use =
        item_controller.useInventoryItem(
            0,
            medicine_inventory,
            items,
            item_use_targets(inventory_player));
    inventory_player.setCurrentMana(
        inventory_player.baseMaximumMana() - 10);
    const osf::PlayerItemUseResult inventory_capsule_use =
        item_controller.useInventoryItem(
            0,
            medicine_inventory,
            items,
            item_use_targets(inventory_player));
    if (!check(
            inventory_tablet_use.consumed &&
                inventory_tablet_use.sound_sample == 16 &&
                inventory_capsule_use.consumed &&
                inventory_capsule_use.sound_sample == 16 &&
                medicine_inventory.items().empty() &&
                inventory_player.currentLife() ==
                    inventory_player.baseMaximumLife() &&
                inventory_player.currentMana() ==
                    inventory_player.baseMaximumMana(),
            "Right-click backpack medicine use differs from belt use.")) {
        return 1;
    }

    osf::PlayerInventory boosted_medicine_inventory;
    osf::PlayerData boosted_player = male;
    boosted_player.setCurrentLife(100, 500);
    if (!check(
            boosted_medicine_inventory.add(*tablet) &&
                item_controller
                    .useInventoryItem(
                        0,
                        boosted_medicine_inventory,
                        items,
                        {
                            boosted_player,
                            500,
                            boosted_player.baseMaximumMana(),
                            50,
                            0,
                            nullptr,
                        })
                    .consumed &&
                boosted_player.currentLife() == 400,
            "Equipped maximum-life bonuses did not scale medicine by "
            "the retail order of operations.")) {
        return 1;
    }

    osf::PlayerData condition_player = male;
    osf::PlayerInventory condition_inventory;
    if (!check(
            condition_inventory.add(*fire_medicine) &&
                condition_inventory.add(*white_medicine) &&
                item_controller
                    .useInventoryItem(
                        0,
                        condition_inventory,
                        items,
                        item_use_targets(condition_player))
                    .consumed &&
                condition_player.elementX() == 0 &&
                condition_player.elementY() == 4000 &&
                item_controller
                    .useInventoryItem(
                        0,
                        condition_inventory,
                        items,
                        item_use_targets(condition_player))
                    .consumed &&
                condition_player.elementX() == 0 &&
                condition_player.elementY() == 0 &&
                condition_inventory.add(*white_medicine) &&
                !item_controller
                     .useInventoryItem(
                         0,
                         condition_inventory,
                         items,
                         item_use_targets(condition_player))
                     .consumed &&
                condition_inventory.items().size() == 1,
            "Elemental and White Medicine did not move or clear the "
            "saved retail condition axes transactionally.")) {
        return 1;
    }
    for (int use = 0; use < 5; ++use) {
        if (!condition_player.applyElementMedicine(0, 4000)) {
            return 1;
        }
    }
    if (!check(
            condition_player.elementX() == 0 &&
                condition_player.elementY() == 20000 &&
                !condition_player.applyElementMedicine(0, 4000),
            "Elemental Medicine did not stop exactly on its retail "
            "element anchor.")) {
        return 1;
    }

    const std::filesystem::path new_save_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_new_save_test";
    const std::filesystem::path new_save_path =
        new_save_root / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(
        new_save_root, cleanup_error);
    male.setCompanionRespawnCounter(600);
    if (!check(
            male.applyElementMedicine(0, 4000),
            "The save fixture could not acquire an elemental condition.")) {
        return 1;
    }
    if (!check(
            osf::writeRetailSave(
                new_save_path, male, 0x34, &error),
            "A new save could not be written into a missing "
            "save directory.")) {
        std::cerr << error << '\n';
        return 1;
    }
    osf::PlayerData new_save_round_trip;
    if (!check(
            new_save_round_trip.loadRetailSave(
                new_save_path, &error) &&
                new_save_round_trip.retailRecord() ==
                    male.retailRecord() &&
                new_save_round_trip.elementY() == 4000 &&
                new_save_round_trip.companionRespawnCounter() ==
                    600,
            "A newly created save did not preserve its player "
            "record or companion respawn countdown.")) {
        std::cerr << error << '\n';
        return 1;
    }

    const osf::ItemDefinition* dagger =
        items.find(0, 0);
    const osf::ItemDefinition* leather_cloth =
        items.find(1, 0);
    const osf::ItemDefinition* gold =
        items.find(4, 0);
    const osf::ItemDefinition* spirit_stone =
        items.find(4, 98000001);
    const osf::TableData* repair_values = tables.find(34);
    const osf::ItemDefinition* shield = nullptr;
    for (const osf::ItemDefinition& definition :
         items.definitions(1)) {
        if (definition.subtype == 2 &&
            definition.required_level <= male.level()) {
            shield = &definition;
            break;
        }
    }
    osf::InventoryItem repair_dagger =
        dagger
            ? osf::makeInventoryItem(*dagger)
            : osf::InventoryItem{};
    if (dagger) {
        repair_dagger.retail_state.resize(200);
        repair_dagger.durability =
            dagger->maximum_durability / 2;
        writeU32(
            repair_dagger.retail_state,
            47u * 4u,
            static_cast<std::uint32_t>(
                repair_dagger.durability));
    }
    const std::int32_t expected_dagger_value =
        dagger && repair_values
            ? dagger->base_price + repair_values->value(24, 0)
            : 0;
    const std::int32_t expected_dagger_repair_price =
        dagger
            ? std::max(
                  1,
                  ((dagger->maximum_durability -
                    repair_dagger.durability) *
                   (expected_dagger_value / 10)) /
                      dagger->maximum_durability)
            : 0;
    if (!check(
            dagger && shield && repair_values &&
                osf::retailItemValue(
                    repair_dagger, *dagger, *repair_values) ==
                    expected_dagger_value &&
                osf::retailItemRepairPrice(
                    repair_dagger, *dagger, *repair_values) ==
                    expected_dagger_repair_price &&
                osf::repairInventoryItem(
                    repair_dagger, *dagger) &&
                repair_dagger.durability ==
                    dagger->maximum_durability &&
                readU32(
                    repair_dagger.retail_state,
                    47u * 4u) ==
                    static_cast<std::uint32_t>(
                        dagger->maximum_durability),
            "The retail item-value, repair-price, or durability mutation "
            "formula differs from the executable.")) {
        return 1;
    }

    osf::InventoryItem damaged_active = repair_dagger;
    damaged_active.durability = dagger->maximum_durability / 2;
    osf::InventoryItem damaged_alternate = repair_dagger;
    damaged_alternate.durability = dagger->maximum_durability / 4;
    osf::PlayerEquipment repair_equipment;
    if (!check(
            repair_equipment
                    .place(
                        osf::EquipmentSlot::main_hand,
                        damaged_active,
                        *dagger,
                        male.level())
                    .accepted &&
                repair_equipment
                    .place(
                        osf::EquipmentSlot::alternate_main_hand,
                        damaged_alternate,
                        *dagger,
                        male.level())
                    .accepted &&
                repair_equipment.repairPrice(
                    osf::EquipmentRepairGroup::arms,
                    items,
                    *repair_values) ==
                    osf::retailItemRepairPrice(
                        damaged_active, *dagger, *repair_values) +
                        osf::retailItemRepairPrice(
                            damaged_alternate,
                            *dagger,
                            *repair_values) &&
                repair_equipment.repair(
                    osf::EquipmentRepairGroup::arms,
                    items) == 2 &&
                repair_equipment
                        .item(osf::EquipmentSlot::main_hand)
                        ->durability == dagger->maximum_durability &&
                repair_equipment
                        .item(
                            osf::EquipmentSlot::alternate_main_hand)
                        ->durability == dagger->maximum_durability,
            "The Arms repair group did not include both retail weapon "
            "sets.")) {
        return 1;
    }

    osf::PlayerInventory repair_inventory;
    const std::int32_t damaged_price =
        osf::retailItemRepairPrice(
            damaged_active, *dagger, *repair_values);
    if (!check(
            repair_inventory.store(damaged_active) &&
                repair_inventory.add(*tablet) &&
                repair_inventory.repairPrice(
                    items, *repair_values) == damaged_price &&
                repair_inventory.repairAll(items) == 1 &&
                repair_inventory.items().size() == 2 &&
                repair_inventory.items()[0].durability ==
                    dagger->maximum_durability &&
                repair_inventory.items()[1].category == 3,
            "Non-Equipped repair did not limit itself to damaged weapons "
            "and armor in the backpack.")) {
        return 1;
    }

    osf::PlayerInventory saved_inventory;
    osf::PlayerEquipment saved_equipment;
    osf::PlayerBelt saved_belt;
    osf::PlayerSpecialItems saved_special_items;
    osf::RetailRandom dropped_item_random(73);
    const osf::InventoryItem dropped_dagger =
        dagger
            ? osf::makeRetailInventoryItem(
                  *dagger,
                  [&dropped_item_random]() {
                      return dropped_item_random.next();
                  })
            : osf::InventoryItem{};
    if (!check(
            dagger &&
                leather_cloth &&
                gold &&
                saved_inventory.store(dropped_dagger) &&
                saved_inventory.add(*gold, 250) &&
                saved_special_items
                    .place(
                        osf::makeInventoryItem(*gold, 75),
                        3,
                        2)
                    .accepted,
            "The save item round-trip fixture could not be created.")) {
        return 1;
    }
    std::optional<osf::InventoryItem> carried_dagger =
        saved_inventory.take(0);
    if (!check(
            carried_dagger &&
                saved_inventory
                    .place(
                        std::move(*carried_dagger),
                        4,
                        0)
                    .accepted &&
                saved_equipment
                    .place(
                        osf::EquipmentSlot::body,
                        osf::makeInventoryItem(*leather_cloth),
                        *leather_cloth,
                        male.level())
                    .accepted &&
                saved_equipment.setAppearanceColor(
                    osf::EquipmentSlot::body, 14) &&
                saved_equipment
                    .place(
                        osf::EquipmentSlot::alternate_main_hand,
                        repair_dagger,
                        *dagger,
                        male.level())
                    .accepted &&
                saved_equipment
                    .place(
                        osf::EquipmentSlot::alternate_off_hand,
                        osf::makeInventoryItem(*shield),
                        *shield,
                        male.level())
                    .accepted &&
                saved_belt
                    .place(
                        osf::makeInventoryItem(*tablet),
                        2,
                        0,
                        *tablet)
                    .accepted,
            "The positioned save item fixture could not be created.")) {
        return 1;
    }
    if (!check(
            osf::writeRetailSave(
                new_save_path,
                male,
                items,
                saved_inventory,
                saved_equipment,
                saved_belt,
                saved_special_items,
                0x34,
                &error),
            "Owned items could not be written to the retail payload.")) {
        std::cerr << error << '\n';
        return 1;
    }
    std::vector<std::uint8_t> owned_item_payload;
    osf::PlayerInventory restored_inventory;
    osf::PlayerEquipment restored_equipment;
    osf::PlayerBelt restored_belt;
    osf::PlayerSpecialItems restored_special_items;
    if (!check(
            osf::readRetailSavePayload(
                new_save_path,
                owned_item_payload,
                &error) &&
                osf::restoreRetailOwnedItems(
                    owned_item_payload,
                    items,
                    male.level(),
                    restored_inventory,
                    restored_equipment,
                    restored_belt,
                    restored_special_items,
                    nullptr,
                    &error) &&
                restored_inventory.items().size() == 2 &&
                restored_inventory.itemAt(4, 0) &&
                restored_inventory.itemAt(4, 0)->category == 0 &&
                restored_inventory.itemAt(4, 0)->identified == 1 &&
                restored_inventory.itemAt(4, 0)->retail_state ==
                    dropped_dagger.retail_state &&
                restored_inventory.gold() == 250 &&
                restored_equipment.item(
                    osf::EquipmentSlot::body) &&
                restored_equipment
                        .item(osf::EquipmentSlot::body)
                        ->definition_id == 0 &&
                restored_equipment
                        .item(osf::EquipmentSlot::body)
                        ->identified == 1 &&
                osf::retailItemColorIndex(
                    *restored_equipment.item(
                        osf::EquipmentSlot::body)) == 14 &&
                restored_equipment.item(
                    osf::EquipmentSlot::alternate_main_hand) &&
                restored_equipment
                        .item(
                            osf::EquipmentSlot::alternate_main_hand)
                        ->definition_id == dagger->id &&
                restored_equipment
                        .item(
                            osf::EquipmentSlot::alternate_main_hand)
                        ->durability == dagger->maximum_durability &&
                restored_equipment.item(
                    osf::EquipmentSlot::alternate_off_hand) &&
                restored_equipment
                        .item(
                            osf::EquipmentSlot::alternate_off_hand)
                        ->definition_id == shield->id &&
                restored_belt.itemAt(2, 0) &&
                restored_belt.itemAt(2, 0)->category == 3 &&
                restored_belt.itemAt(2, 0)->definition_id == 0 &&
                restored_special_items.items().size() == 1 &&
                restored_special_items.items()[0].category == 4 &&
                restored_special_items.items()[0].quantity == 75 &&
                restored_special_items.items()[0].grid_x == 3 &&
                restored_special_items.items()[0].grid_y == 2,
            "Backpack, equipment, belt, or special-item ownership did not "
            "survive a save/load round trip.")) {
        std::cerr << error << '\n';
        return 1;
    }

    osf::PlayerGiantWarehouse saved_giant_warehouse;
    saved_giant_warehouse.initializeNew();
    osf::PlayerGiantWarehouse::EnabledFlags giant_flags{};
    giant_flags[0] = 1;
    giant_flags[3] = 1;
    saved_giant_warehouse.restoreEnabledFlags(giant_flags);
    if (!saved_giant_warehouse.page(3)
             .place(dropped_dagger, 4, 5)
             .accepted) {
        return 1;
    }
    std::vector<std::uint8_t> portable_warehouse_payload;
    osf::replaceRetailSavePortableExtension(
        portable_warehouse_payload, true, 7, true);
    osf::PlayerGiantWarehouse restored_giant_warehouse;
    restored_giant_warehouse.initializeNew();
    if (!check(
            osf::replaceRetailGiantWarehouse(
                portable_warehouse_payload,
                0,
                items,
                saved_giant_warehouse,
                nullptr,
                &error) &&
                osf::restoreRetailGiantWarehouse(
                    portable_warehouse_payload,
                    0,
                    items,
                    restored_giant_warehouse,
                    nullptr,
                    &error) &&
                osf::inspectRetailSavePortableExtension(
                    portable_warehouse_payload)
                    .running &&
                osf::inspectRetailSavePortableExtension(
                    portable_warehouse_payload)
                        .mine_count == 7 &&
                restored_giant_warehouse.pageEnabled(0) &&
                restored_giant_warehouse.pageEnabled(3) &&
                !restored_giant_warehouse.pageEnabled(1) &&
                restored_giant_warehouse.page(3).items().size() == 1 &&
                restored_giant_warehouse.page(3).items()[0].definition_id ==
                    dropped_dagger.definition_id &&
                restored_giant_warehouse.page(3).items()[0].grid_x == 4 &&
                restored_giant_warehouse.page(3).items()[0].grid_y == 5,
            "Giant Warehouse pages, unlocks, running state, or mine count "
            "did not survive the portable save extension.")) {
        std::cerr << error << '\n';
        return 1;
    }

    osf::PlayerAutomaticItems saved_automatic_items;
    osf::PlayerAutomaticItems restored_automatic_items;
    const osf::InventoryItem automatic_spirit_stone =
        spirit_stone
            ? osf::makeRetailInventoryItem(
                  *spirit_stone,
                  [&dropped_item_random]() {
                      return dropped_item_random.next();
                  })
            : osf::InventoryItem{};
    if (!check(
            spirit_stone &&
                saved_automatic_items.add(
                    *spirit_stone, automatic_spirit_stone) &&
                osf::replaceRetailAutomaticItems(
                    portable_warehouse_payload,
                    0,
                    items,
                    saved_automatic_items,
                    nullptr,
                    &error) &&
                osf::replaceRetailGiantWarehouse(
                    portable_warehouse_payload,
                    0,
                    items,
                    saved_giant_warehouse,
                    nullptr,
                    &error) &&
                osf::restoreRetailAutomaticItems(
                    portable_warehouse_payload,
                    0,
                    items,
                    restored_automatic_items,
                    nullptr,
                    &error) &&
                restored_automatic_items.contains(4, 98000001) &&
                restored_automatic_items.page(2).items().size() == 1 &&
                restored_automatic_items.page(2).items()[0].grid_x == 1 &&
                restored_automatic_items.page(2).items()[0].grid_y == 0,
            "Automatic item pages did not survive the portable late-item "
            "save stream.")) {
        std::cerr << error << '\n';
        return 1;
    }

    const osf::ItemDefinition* unknown_sword =
        items.find(0, 10);
    osf::PlayerInventory identified_inventory;
    osf::PlayerEquipment empty_equipment;
    osf::PlayerBelt empty_belt;
    osf::PlayerSpecialItems empty_special_items;
    const std::filesystem::path identify_save_path =
        new_save_root / "Save" / "0001.Ssv";
    if (!check(
            unknown_sword &&
                unknown_sword->variant == 1 &&
                identified_inventory.add(*unknown_sword) &&
                identified_inventory.items()[0].identified == 0 &&
                identified_inventory.identify(0) &&
                osf::writeRetailSave(
                    identify_save_path,
                    male,
                    items,
                    identified_inventory,
                    empty_equipment,
                    empty_belt,
                    empty_special_items,
                    0x34,
                    &error),
            "An Identify-mutated item could not be written to a retail "
            "save.")) {
        std::cerr << error << '\n';
        return 1;
    }
    std::vector<std::uint8_t> identify_payload;
    osf::PlayerInventory restored_identified_inventory;
    if (!check(
            osf::readRetailSavePayload(
                identify_save_path,
                identify_payload,
                &error) &&
                osf::restoreRetailOwnedItems(
                    identify_payload,
                    items,
                    male.level(),
                    restored_identified_inventory,
                    empty_equipment,
                    empty_belt,
                    empty_special_items,
                    nullptr,
                    &error) &&
                restored_identified_inventory.items().size() == 1 &&
                restored_identified_inventory.items()[0].definition_id ==
                    unknown_sword->id &&
                restored_identified_inventory.items()[0].identified == 1 &&
                restored_identified_inventory.items()[0]
                        .retail_state.size() >= 49u * 4u &&
                restored_identified_inventory.items()[0]
                        .retail_state[48u * 4u] == 1,
            "The Identify flag or its raw retail word did not survive "
            "save and load.")) {
        std::cerr << error << '\n';
        return 1;
    }

    const std::filesystem::path retail_save_fixture =
        std::string(OPENSHADOWFLARE_SOURCE_DIR) +
        "/tmp/ShadowFlare/Save/0004.Ssv";
    if (std::filesystem::exists(retail_save_fixture)) {
        osf::PlayerData retail_fixture_player;
        std::vector<std::uint8_t> retail_fixture_payload;
        osf::PlayerInventory retail_fixture_inventory;
        osf::PlayerEquipment retail_fixture_equipment;
        osf::PlayerBelt retail_fixture_belt;
        osf::PlayerSpecialItems retail_fixture_special_items;
        std::size_t retail_owned_items_end = 0;
        if (!check(
                retail_fixture_player.loadRetailSave(
                    retail_save_fixture,
                    &error) &&
                    osf::readRetailSavePayload(
                        retail_save_fixture,
                        retail_fixture_payload,
                        &error) &&
                    osf::restoreRetailOwnedItems(
                        retail_fixture_payload,
                        items,
                        retail_fixture_player.level(),
                        retail_fixture_inventory,
                        retail_fixture_equipment,
                        retail_fixture_belt,
                        retail_fixture_special_items,
                        &retail_owned_items_end,
                        &error),
                "The original retail item stream could not be restored.")) {
            std::cerr << error << '\n';
            return 1;
        }
        osf::TransportCatalog retail_transports;
        if (!check(
                retail_transports.load(tables, &error),
                "The retail transport catalog could not be loaded.")) {
            std::cerr << error << '\n';
            return 1;
        }
        std::vector<std::int32_t> transport_flags =
            retail_transports.enabledFlags();
        if (!check(
                osf::restoreRetailTransportFlags(
                    retail_fixture_payload,
                    retail_owned_items_end,
                    transport_flags,
                    &error) &&
                    transport_flags.size() == 51 &&
                    transport_flags.front() != 0,
                "The original retail transport flags could not be "
                "restored after the owned-item stream.")) {
            std::cerr << error << '\n';
            return 1;
        }
        osf::RetailSaveProgress retail_progress;
        std::size_t retail_progress_end = 0;
        osf::PlayerMagic retail_magic;
        retail_magic.initializeNew();
        std::size_t retail_magic_end = 0;
        std::size_t retail_companion_end = 0;
        std::int32_t retail_mine_count = 5;
        std::size_t retail_mine_end = 0;
        osf::RetailSaveWorldState retail_world_state;
        std::size_t retail_world_state_end = 0;
        osf::PlayerGiantWarehouse retail_giant_warehouse;
        retail_giant_warehouse.initializeNew();
        std::size_t retail_giant_end = retail_mine_end;
        osf::PlayerAutomaticItems retail_automatic_items;
        if (!check(
                osf::restoreRetailProgress(
                    retail_fixture_payload,
                    retail_owned_items_end,
                    retail_progress,
                    &retail_progress_end,
                    &error) &&
                    osf::restoreRetailMagic(
                        retail_fixture_payload,
                        retail_progress_end,
                        retail_magic,
                        &retail_magic_end,
                        &error) &&
                    osf::restoreRetailCompanionProgress(
                        retail_fixture_payload,
                        retail_magic_end,
                        retail_fixture_player,
                        &retail_companion_end,
                        &error) &&
                    osf::restoreRetailMineCount(
                        retail_fixture_payload,
                        retail_companion_end,
                        retail_mine_count,
                        &retail_mine_end,
                        &error) &&
                    osf::restoreRetailWorldState(
                        retail_fixture_payload,
                        retail_mine_end,
                        retail_world_state,
                        &retail_world_state_end,
                        &error) &&
                    osf::restoreRetailGiantWarehouse(
                        retail_fixture_payload,
                        retail_world_state_end,
                        items,
                        retail_giant_warehouse,
                        &retail_giant_end,
                        &error) &&
                    osf::restoreRetailAutomaticItems(
                        retail_fixture_payload,
                        retail_giant_end,
                        items,
                        retail_automatic_items,
                        nullptr,
                        &error) &&
                    retail_mine_count >= 0 &&
                    retail_world_state.running &&
                    retail_world_state.scenario_id == 0 &&
                    retail_world_state.entry_value == 0 &&
                    retail_fixture_player.companionCount() ==
                        companion_count,
                "The original retail companion progression, Mine count, "
                "world state, or ten-page Giant Warehouse could not be "
                "restored after magic.")) {
            std::cerr << error << '\n';
            return 1;
        }
        std::vector<std::uint8_t> rewritten_retail_payload =
            retail_fixture_payload;
        if (!check(
                osf::replaceRetailOwnedItems(
                    rewritten_retail_payload,
                    items,
                    retail_fixture_inventory,
                    retail_fixture_equipment,
                    retail_fixture_belt,
                    retail_fixture_special_items,
                    nullptr,
                    &error) &&
                    rewritten_retail_payload ==
                        retail_fixture_payload,
                "Re-encoding unchanged retail-owned items altered "
                "their bytes or an unowned payload section.")) {
            std::cerr << error << '\n';
            return 1;
        }
        std::vector<std::uint8_t> rewritten_late_items =
            retail_fixture_payload;
        std::size_t rewritten_companion_end = retail_magic_end;
        std::size_t rewritten_world_state_end = retail_mine_end;
        std::size_t rewritten_giant_end = retail_world_state_end;
        if (!check(
                osf::replaceRetailCompanionProgress(
                    rewritten_late_items,
                    retail_magic_end,
                    retail_fixture_player,
                    &rewritten_companion_end,
                    &error) &&
                rewritten_companion_end ==
                    retail_companion_end &&
                osf::replaceRetailWorldState(
                    rewritten_late_items,
                    retail_mine_end,
                    retail_world_state,
                    &rewritten_world_state_end,
                    &error) &&
                rewritten_world_state_end ==
                    retail_world_state_end &&
                osf::replaceRetailGiantWarehouse(
                    rewritten_late_items,
                    rewritten_world_state_end,
                    items,
                    retail_giant_warehouse,
                    &rewritten_giant_end,
                    &error) &&
                osf::replaceRetailAutomaticItems(
                    rewritten_late_items,
                    rewritten_giant_end,
                    items,
                    retail_automatic_items,
                    nullptr,
                    &error) &&
                rewritten_late_items == retail_fixture_payload,
            "Re-encoding retail world state, Giant Warehouse, and four "
            "automatic-item pages altered their bytes or later state.")) {
            std::cerr << error << '\n';
            return 1;
        }
    }

    constexpr std::int32_t surface_width = 640;
    constexpr std::int32_t surface_height = 480;
    std::vector<osf::gapi::Color> surface_pixels(
        static_cast<std::size_t>(surface_width) *
        static_cast<std::size_t>(surface_height));
    for (std::int32_t y = 0; y < surface_height; ++y) {
        for (std::int32_t x = 0; x < surface_width; ++x) {
            surface_pixels[
                static_cast<std::size_t>(y) * surface_width +
                static_cast<std::size_t>(x)] = {
                static_cast<std::uint8_t>(x),
                static_cast<std::uint8_t>(y),
                static_cast<std::uint8_t>(x + y),
                255,
            };
        }
    }
    osf::RetailSavePreview preview;
    preview.capture(
        {
            surface_pixels.data(),
            surface_width,
            surface_height,
        });
    if (!check(
            preview.writeForSave(new_save_path, &error),
            "The retail save preview could not be written.")) {
        std::cerr << error << '\n';
        return 1;
    }
    osf::gapi::BitmapImage preview_image;
    const std::filesystem::path preview_path =
        new_save_root / "Save" / "0000.Bmp";
    const osf::gapi::Color expected_first =
        surface_pixels[183 * surface_width + 124];
    if (!check(
            preview_image.load(preview_path, &error) &&
                preview_image.width() ==
                    osf::RetailSavePreview::width &&
                preview_image.height() ==
                    osf::RetailSavePreview::height &&
                preview_image.pixels().front().red ==
                    expected_first.red &&
                preview_image.pixels().front().green ==
                    expected_first.green &&
                preview_image.pixels().front().blue ==
                    expected_first.blue,
            "The saved preview dimensions, crop, or BMP "
            "orientation differ.")) {
        std::cerr << error << '\n';
        return 1;
    }
    std::filesystem::remove_all(
        new_save_root, cleanup_error);

    osf::PlayerData female;
    if (!check(
            female.initializeNew(
                "Faye",
                osf::playerGenderValue(osf::PlayerGender::female),
                tables,
                &error) &&
                female.gender() == 0 &&
                female.baseMaximumLife() == 140 &&
                female.currentLife() == 140 &&
                female.baseMaximumMana() == 160 &&
                female.currentMana() == 160 &&
                female.walkingSpeedTier() == 5,
            "The female record does not match table 901.")) {
        return 1;
    }
    female.setCurrentLife(1);
    female.setCurrentMana(2);
    female.restoreForRespawn();
    if (!check(
            female.currentLife() == female.baseMaximumLife() &&
                female.currentMana() ==
                    female.baseMaximumMana(),
            "The retail revive reset did not restore both player "
            "resource pools.")) {
        return 1;
    }

    std::array<std::uint8_t, osf::PlayerData::retail_record_size>
        saved_record{};
    const std::string saved_name = "SavedHero";
    std::copy(
        saved_name.begin(), saved_name.end(), saved_record.begin());
    writeI32(saved_record, 0x18, 1);
    writeI32(saved_record, 0x1c, 7);
    writeI32(saved_record, 0x24, 22);
    writeI32(saved_record, 0x30, 321);
    writeI32(saved_record, 0x34, 123);
    writeI32(saved_record, 0x38, 456);
    writeI32(saved_record, 0x3c, 234);
    saved_record[0x100] = 0xa5;

    const std::filesystem::path save_path =
        std::filesystem::temp_directory_path() /
        "openshadowflare_player_data_test.ssv";
    {
        std::ofstream stream(save_path, std::ios::binary);
        const std::array<char, 16> signature{{
            'S', 'h', 'a', 'd', 'o', 'w', 'F', 'l',
            'a', 'r', 'e', '0', '0', '0', '5', '\0',
        }};
        stream.write(signature.data(), signature.size());
        stream.write(
            reinterpret_cast<const char*>(saved_record.data()),
            static_cast<std::streamsize>(saved_record.size()));
    }

    osf::PlayerData loaded;
    const bool loaded_ok =
        loaded.loadRetailSave(save_path, &error);
    if (!check(
            loaded_ok &&
                loaded.name() == saved_name &&
                loaded.gender() == 1 &&
                loaded.job() == 7 &&
                loaded.level() == 22 &&
                loaded.baseMaximumLife() == 321 &&
                loaded.currentLife() == 123 &&
                loaded.baseMaximumMana() == 456 &&
                loaded.currentMana() == 234 &&
                loaded.retailRecord()[0x100] == 0xa5,
            "The retail 0x160-byte save record was not preserved.")) {
        std::cerr << error << '\n';
        return 1;
    }

    if (!check(
            osf::writeRetailSave(
                save_path, loaded, 0x34, &error),
            "The retail save envelope could not be written.")) {
        std::cerr << error << '\n';
        return 1;
    }
    std::vector<std::uint8_t> saved_bytes;
    {
        std::ifstream stream(save_path, std::ios::binary);
        saved_bytes.assign(
            std::istreambuf_iterator<char>(stream),
            std::istreambuf_iterator<char>());
    }
    std::int32_t checksum = 0;
    for (std::uint8_t value : saved_record) {
        checksum += static_cast<std::int8_t>(value);
    }
    if (!check(
            saved_bytes.size() ==
                16 + osf::PlayerData::retail_record_size +
                    9 + osf::PlayerData::retail_record_size &&
                readU32(saved_bytes, 0x170) ==
                    osf::PlayerData::retail_record_size &&
                saved_bytes[0x174] == 0x34 &&
                static_cast<std::int32_t>(
                    readU32(saved_bytes, 0x175)) == checksum &&
                saved_bytes[0x179] == 0xc4 &&
                saved_bytes[0x179 + 9] == 0xee,
            "The save size, XOR key, checksum, or substitution "
            "encoding differs from 0x0044b580.")) {
        return 1;
    }

    std::vector<std::uint8_t> extended_save = saved_bytes;
    writeU32(
        extended_save,
        0x170,
        osf::PlayerData::retail_record_size + 1);
    writeU32(
        extended_save,
        0x175,
        static_cast<std::uint32_t>(checksum + 42));
    // With key 0x34, substitution index 0x27 decodes to the
    // deliberately unknown trailing payload byte 0x2a.
    extended_save.push_back(0x27);
    {
        std::ofstream stream(
            save_path, std::ios::binary | std::ios::trunc);
        stream.write(
            reinterpret_cast<const char*>(extended_save.data()),
            static_cast<std::streamsize>(extended_save.size()));
    }
    if (!check(
            osf::writeRetailSave(
                save_path, loaded, 0x34, &error),
            "An extended retail payload could not be preserved.")) {
        std::cerr << error << '\n';
        return 1;
    }
    std::vector<std::uint8_t> preserved_bytes;
    {
        std::ifstream stream(save_path, std::ios::binary);
        preserved_bytes.assign(
            std::istreambuf_iterator<char>(stream),
            std::istreambuf_iterator<char>());
    }
    if (!check(
            preserved_bytes == extended_save,
            "An unknown retail payload byte changed during save.")) {
        return 1;
    }

    std::vector<std::uint8_t> corrupt_save = preserved_bytes;
    corrupt_save[0x175] ^= 0x01;
    {
        std::ofstream stream(
            save_path, std::ios::binary | std::ios::trunc);
        stream.write(
            reinterpret_cast<const char*>(corrupt_save.data()),
            static_cast<std::streamsize>(corrupt_save.size()));
    }
    if (!check(
            !osf::writeRetailSave(
                save_path, loaded, 0x34, &error),
            "A corrupt source payload was unexpectedly replaced.")) {
        return 1;
    }
    std::vector<std::uint8_t> rejected_bytes;
    {
        std::ifstream stream(save_path, std::ios::binary);
        rejected_bytes.assign(
            std::istreambuf_iterator<char>(stream),
            std::istreambuf_iterator<char>());
    }
    if (!check(
            rejected_bytes == corrupt_save,
            "Rejecting a corrupt save changed its source bytes.")) {
        return 1;
    }
    {
        std::ofstream stream(
            save_path, std::ios::binary | std::ios::trunc);
        stream.write(
            reinterpret_cast<const char*>(preserved_bytes.data()),
            static_cast<std::streamsize>(preserved_bytes.size()));
    }

    osf::PlayerData round_trip;
    const bool round_trip_ok =
        round_trip.loadRetailSave(save_path, &error);
    std::error_code remove_error;
    std::filesystem::remove(save_path, remove_error);
    if (!check(
            round_trip_ok &&
                round_trip.retailRecord() ==
                    loaded.retailRecord(),
            "The written save did not preserve its plain player record.")) {
        return 1;
    }
    return 0;
}
