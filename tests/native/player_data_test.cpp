#include "libs/RKC_DIB/rkc_dib.hpp"
#include "world/player_data.hpp"
#include "world/retail_save_file.hpp"
#include "world/retail_save_preview.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
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
                male.walkingSpeedTier() == 5,
            "The male record does not match table 901 and FUN_00440f70.")) {
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
