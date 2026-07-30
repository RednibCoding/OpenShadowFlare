#include "player_data.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <utility>

namespace osf {
namespace {

constexpr std::array<char, 16> kSaveSignature{{
    'S', 'h', 'a', 'd', 'o', 'w', 'F', 'l',
    'a', 'r', 'e', '0', '0', '0', '5', '\0',
}};

constexpr std::array<std::size_t, 13>
    kInitialParameterOffsets{{
        0x28,
        0x2c,
        0x30,
        0x38,
        0x40,
        0x44,
        0x48,
        0x54,
        0x58,
        0x4c,
        0x50,
        0x5c,
        0x60,
    }};

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

}  // namespace

bool PlayerData::initializeNew(
    std::string_view name,
    std::int32_t gender,
    const TableDatabase& tables,
    std::string* error) {
    clear();

    // FUN_00440f70 selects table 901 for the male record and table 900 for
    // the female record. Other values follow the male resource path in the
    // retail executable.
    const std::int32_t normalized_gender = gender == 1 ? 1 : 0;
    const TableData* initial_values =
        tables.find(901 - normalized_gender);
    if (!initial_values ||
        initial_values->rowCount() <
            static_cast<std::int32_t>(
                initial_parameter_count) ||
        initial_values->columnCount() < 1) {
        setError(
            error,
            "The new-character parameter table is missing.");
        return false;
    }

    const std::size_t name_length =
        std::min<std::size_t>(name.size(), 23);
    if (name_length != 0) {
        std::memcpy(record_.data(), name.data(), name_length);
    }
    writeI32(0x18, normalized_gender);

    // These values are written after the retail function copies the 0x160
    // record. Their broader meanings are intentionally left to later slices.
    writeI32(0x1c, 0x10);
    writeI32(0x20, 0x10);
    writeI32(0x24, 1);

    for (std::size_t row = 0;
         row < initial_parameter_count;
         ++row) {
        writeI32(
            kInitialParameterOffsets[row],
            initial_values->value(
                static_cast<std::int32_t>(row), 0));
    }
    writeI32(0x34, readI32(0x30));
    writeI32(0x3c, readI32(0x38));

    // The last part of the persistent record stores the initial companion
    // selection/progression fields initialized by FUN_00440f70.
    writeI32(0x140, 0);
    writeI32(0x144, 1);
    writeI32(0x148, 0);
    writeI32(0x14c, 0);
    writeI32(0x150, 10);
    writeI32(0x154, 0);
    writeI32(0x158, 0);

    valid_ = true;
    return true;
}

bool PlayerData::loadRetailSave(
    const std::filesystem::path& path,
    std::string* error) {
    clear();

    std::ifstream stream(path, std::ios::binary);
    std::array<char, 16> signature{};
    if (!stream ||
        !stream.read(signature.data(), signature.size()) ||
        signature != kSaveSignature ||
        !stream.read(
            reinterpret_cast<char*>(record_.data()),
            static_cast<std::streamsize>(record_.size()))) {
        setError(
            error,
            "The retail save header is invalid or truncated.");
        clear();
        return false;
    }
    valid_ = true;
    return true;
}

bool PlayerData::load(
    const PlayerLoadRequest& request,
    const TableDatabase& tables,
    std::string* error) {
    if (request.source == PlayerDataSource::retail_save) {
        return loadRetailSave(request.save_path, error);
    }
    return initializeNew(
        request.name, request.gender, tables, error);
}

void PlayerData::clear() {
    record_.fill(0);
    valid_ = false;
}

bool PlayerData::valid() const {
    return valid_;
}

std::string PlayerData::name() const {
    std::size_t length = 0;
    while (length < 24 && record_[length] != 0) {
        ++length;
    }
    return std::string(
        reinterpret_cast<const char*>(record_.data()),
        length);
}

std::int32_t PlayerData::gender() const {
    return readI32(0x18);
}

std::int32_t PlayerData::job() const {
    return readI32(0x1c);
}

std::int32_t PlayerData::level() const {
    return readI32(0x24);
}

std::int32_t PlayerData::baseMaximumLife() const {
    return readI32(0x30);
}

std::int32_t PlayerData::currentLife() const {
    return readI32(0x34);
}

std::int32_t PlayerData::baseMaximumMana() const {
    return readI32(0x38);
}

std::int32_t PlayerData::currentMana() const {
    return readI32(0x3c);
}

void PlayerData::setCurrentLife(std::int32_t value) {
    writeI32(
        0x34,
        std::clamp(
            value,
            0,
            std::max(0, baseMaximumLife())));
}

void PlayerData::setCurrentMana(std::int32_t value) {
    writeI32(
        0x3c,
        std::clamp(
            value,
            0,
            std::max(0, baseMaximumMana())));
}

bool PlayerData::restoreLife(
    std::int32_t amount,
    std::int32_t maximum_percent) {
    const std::int32_t before = currentLife();
    const std::int64_t restored =
        static_cast<std::int64_t>(before) +
        amount +
        static_cast<std::int64_t>(maximum_percent) *
            baseMaximumLife() /
            100;
    setCurrentLife(
        static_cast<std::int32_t>(
            std::clamp<std::int64_t>(
                restored,
                0,
                std::max(0, baseMaximumLife()))));
    return currentLife() != before;
}

bool PlayerData::restoreMana(
    std::int32_t amount,
    std::int32_t maximum_percent) {
    const std::int32_t before = currentMana();
    const std::int64_t restored =
        static_cast<std::int64_t>(before) +
        amount +
        static_cast<std::int64_t>(maximum_percent) *
            baseMaximumMana() /
            100;
    setCurrentMana(
        static_cast<std::int32_t>(
            std::clamp<std::int64_t>(
                restored,
                0,
                std::max(0, baseMaximumMana()))));
    return currentMana() != before;
}

std::int32_t PlayerData::initialParameter(
    std::size_t row) const {
    return row < kInitialParameterOffsets.size()
        ? readI32(kInitialParameterOffsets[row])
        : 0;
}

std::int32_t PlayerData::companionType() const {
    // FUN_00433692 reads runtime offset 0x150. The persistent 0x160-byte
    // player record starts at runtime offset 0x10, so the value belongs to
    // record offset 0x140 and survives retail saves.
    return readI32(0x140);
}

std::int32_t PlayerData::baseAttackSpeed() const {
    // FUN_00440f70 stores table 900/901 row zero at runtime offset
    // 0x38 (persistent record offset 0x28). FUN_0044ea60 copies it to
    // the derived attack-speed field at 0x198 before equipment is added.
    return initialParameter(0);
}

std::int32_t PlayerData::baseWeightCapacity() const {
    // Row four becomes runtime offset 0x50 and then derived offset
    // 0x1b0, the capacity compared with equipped weight by FUN_00450c60.
    return initialParameter(4);
}

std::int32_t PlayerData::walkingSpeedTier() const {
    // FUN_00450d40 uses the second initial parameter, adds 32, divides by
    // 32, and clamps the resulting movement tier to the retail 0..9 range.
    return std::clamp(
        (initialParameter(1) + 32) / 32,
        0,
        9);
}

const std::array<std::uint8_t, PlayerData::retail_record_size>&
PlayerData::retailRecord() const {
    return record_;
}

std::int32_t PlayerData::readI32(std::size_t offset) const {
    if (offset > record_.size() ||
        record_.size() - offset < 4) {
        return 0;
    }
    const std::uint32_t value =
        static_cast<std::uint32_t>(record_[offset]) |
        (static_cast<std::uint32_t>(record_[offset + 1]) << 8u) |
        (static_cast<std::uint32_t>(record_[offset + 2]) << 16u) |
        (static_cast<std::uint32_t>(record_[offset + 3]) << 24u);
    return static_cast<std::int32_t>(value);
}

void PlayerData::writeI32(
    std::size_t offset,
    std::int32_t value) {
    if (offset > record_.size() ||
        record_.size() - offset < 4) {
        return;
    }
    const std::uint32_t unsigned_value =
        static_cast<std::uint32_t>(value);
    record_[offset] =
        static_cast<std::uint8_t>(unsigned_value);
    record_[offset + 1] =
        static_cast<std::uint8_t>(unsigned_value >> 8u);
    record_[offset + 2] =
        static_cast<std::uint8_t>(unsigned_value >> 16u);
    record_[offset + 3] =
        static_cast<std::uint8_t>(unsigned_value >> 24u);
}

}  // namespace osf
