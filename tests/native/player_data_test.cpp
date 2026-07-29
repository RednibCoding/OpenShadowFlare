#include "world/player_data.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

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
    std::error_code remove_error;
    std::filesystem::remove(save_path, remove_error);
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
    return 0;
}
