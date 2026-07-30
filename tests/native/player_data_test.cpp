#include "items/item_database.hpp"
#include "items/item_instance_factory.hpp"
#include "items/player_belt.hpp"
#include "items/player_equipment.hpp"
#include "items/player_inventory.hpp"
#include "items/player_special_items.hpp"
#include "libs/RKC_DIB/rkc_dib.hpp"
#include "world/player_data.hpp"
#include "world/player_item_controller.hpp"
#include "world/retail_save_file.hpp"
#include "world/retail_save_items.hpp"
#include "world/retail_save_preview.hpp"
#include "world/retail_save_progress.hpp"
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
            male.initializeNew("Mina", 0, tables, &error),
            "A new male character could not be initialized.")) {
        std::cerr << error << '\n';
        return 1;
    }
    if (!check(
            male.valid() &&
                male.name() == "Mina" &&
                male.gender() == 0 &&
                male.job() == 0x10 &&
                male.level() == 1 &&
                male.baseMaximumLife() == 140 &&
                male.currentLife() == 140 &&
                male.baseMaximumMana() == 160 &&
                male.currentMana() == 160 &&
                male.initialParameter(1) == 128 &&
                male.baseMagicalEvasionRate() ==
                    male.initialParameter(12) &&
                male.walkingSpeedTier() == 5,
            "The male record does not match table 901 and FUN_00440f70.")) {
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
    osf::PlayerBelt belt;
    osf::PlayerItemController item_controller;
    item_controller.initializeNew();
    if (!check(
            tablet &&
                capsule &&
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
                     belt_player)
                 .consumed &&
                belt.itemAt(0, 0),
            "A Tablet was consumed while life was already full.")) {
        return 1;
    }
    belt_player.setCurrentLife(0);
    belt_player.setCurrentMana(0);
    if (!check(
            belt_player.restoreLife(0, 10) &&
                belt_player.currentLife() == 14 &&
                belt_player.restoreMana(0, 10) &&
                belt_player.currentMana() == 16,
            "Percentage life or mana restoration differs.")) {
        return 1;
    }
    belt_player.setCurrentLife(
        belt_player.baseMaximumLife() - 10);
    const osf::BeltItemUseResult tablet_use =
        item_controller.useBeltPocket(
            0,
            belt,
            items,
            belt_player);
    belt_player.setCurrentMana(
        belt_player.baseMaximumMana() - 10);
    const osf::BeltItemUseResult capsule_use =
        item_controller.useBeltPocket(
            4,
            belt,
            items,
            belt_player);
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

    const std::filesystem::path new_save_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_new_save_test";
    const std::filesystem::path new_save_path =
        new_save_root / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(
        new_save_root, cleanup_error);
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
                    male.retailRecord(),
            "A newly created save did not preserve its player "
            "record.")) {
        std::cerr << error << '\n';
        return 1;
    }

    const osf::ItemDefinition* dagger =
        items.find(0, 0);
    const osf::ItemDefinition* leather_cloth =
        items.find(1, 0);
    const osf::ItemDefinition* gold =
        items.find(4, 0);
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

    const std::filesystem::path retail_save_fixture =
        std::string(OPENSHADOWFLARE_SOURCE_DIR) +
        "/tmp/ShadowFlare/Save/0003.Ssv";
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
            female.initializeNew("Faye", 1, tables, &error) &&
                female.gender() == 1 &&
                female.baseMaximumLife() == 150 &&
                female.currentLife() == 150 &&
                female.baseMaximumMana() == 150 &&
                female.currentMana() == 150 &&
                female.walkingSpeedTier() == 5,
            "The female record does not match table 900.")) {
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
