#include "player_data.hpp"
#include "player_element_condition.hpp"

#include "core/retail_integer.hpp"

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

constexpr std::array<std::int32_t, 17> kJobGrowthMap{{
    -1, -1, 4, -1, -1, 3, 1, -1, 5,
    2, 6, -1, -1, -1, -1, -1, 0,
}};

constexpr std::size_t kTotalKillCountOffset = 0xb0;
constexpr std::size_t kKillCountOffset = 0xb4;
constexpr std::size_t kKillCountCount = 9;
constexpr std::size_t kExperienceOffset = 0xd8;
constexpr std::size_t kJobHistoryOffset = 0xdc;
constexpr std::size_t kJobHistoryCount = 100;
constexpr std::int32_t kCompanionCatalogTable = 60;

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

    // FUN_00440f70 stores zero for female and one for male, then selects
    // initial parameter table 901 or 900 respectively.
    const std::int32_t normalized_gender =
        gender == playerGenderValue(PlayerGender::male)
            ? playerGenderValue(PlayerGender::male)
            : playerGenderValue(PlayerGender::female);
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
    std::fill_n(
        record_.begin() +
            static_cast<std::ptrdiff_t>(kJobHistoryOffset),
        kJobHistoryCount,
        static_cast<std::uint8_t>(0x10));

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

    if (!initializeCompanionProgress(tables, error)) {
        clear();
        return false;
    }
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
        if (!loadRetailSave(request.save_path, error) ||
            !initializeCompanionProgress(tables, error)) {
            clear();
            return false;
        }
        return true;
    }
    return initializeNew(
        request.name, request.gender, tables, error);
}

void PlayerData::clear() {
    record_.fill(0);
    companion_levels_.clear();
    companion_experiences_.clear();
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

void PlayerData::setJob(PlayerJob job) {
    writeI32(0x1c, playerJobValue(job));
}

std::int32_t PlayerData::level() const {
    return readI32(0x24);
}

std::int32_t PlayerData::experience() const {
    return readI32(kExperienceOffset);
}

std::int32_t PlayerData::experienceThreshold(
    const TableDatabase& tables) const {
    const TableData* thresholds = tables.find(13);
    return level() > 0 &&
               level() < 100 &&
               thresholds &&
               thresholds->contains(level() - 1, 0)
        ? thresholds->value(level() - 1, 0)
        : 0;
}

std::int32_t PlayerData::totalKillCount() const {
    return readI32(kTotalKillCountOffset);
}

std::int32_t PlayerData::killCount(
    std::size_t kind) const {
    return kind < kKillCountCount
        ? readI32(kKillCountOffset + kind * 4u)
        : 0;
}

std::int32_t PlayerData::jobLevel(
    std::int32_t job_value) const {
    std::int32_t count = 0;
    const std::int32_t history_count =
        std::clamp(
            level(), 0,
            static_cast<std::int32_t>(kJobHistoryCount));
    for (std::int32_t index = 0;
         index < history_count;
         ++index) {
        if (static_cast<std::int8_t>(
                record_[
                    kJobHistoryOffset +
                    static_cast<std::size_t>(index)]) ==
            job_value) {
            ++count;
        }
    }
    return count;
}

void PlayerData::addExperience(std::int32_t amount) {
    writeI32(
        kExperienceOffset,
        retailAdd(experience(), amount));
}

void PlayerData::addKillCount(std::size_t kind) {
    writeI32(
        kTotalKillCountOffset,
        retailAdd(totalKillCount(), 1));
    if (kind < kKillCountCount) {
        writeI32(
            kKillCountOffset + kind * 4u,
            retailAdd(killCount(kind), 1));
    }
}

bool PlayerData::applyLevelThreshold(
    const TableDatabase& tables) {
    const std::int32_t current_level = level();
    const std::int32_t threshold =
        experienceThreshold(tables);
    if (current_level <= 0 ||
        current_level >= 100 ||
        threshold <= 0 ||
        experience() < threshold) {
        return false;
    }
    levelUp(tables);
    writeI32(kExperienceOffset, 0);
    setCurrentLife(baseMaximumLife());
    setCurrentMana(baseMaximumMana());
    return true;
}

void PlayerData::levelUp(
    const TableDatabase& tables) {
    const std::int32_t old_level = level();
    const std::int32_t current_job = job();
    if (old_level <= 0 || old_level >= 100 ||
        current_job < 0 ||
        static_cast<std::size_t>(current_job) >=
            kJobGrowthMap.size()) {
        return;
    }

    record_[
        kJobHistoryOffset +
        static_cast<std::size_t>(old_level)] =
        static_cast<std::uint8_t>(current_job);
    const std::int32_t new_level = old_level + 1;
    writeI32(0x24, new_level);

    std::int32_t job_level = 0;
    for (std::int32_t index = 0;
         index < new_level;
         ++index) {
        if (record_[
                kJobHistoryOffset +
                static_cast<std::size_t>(index)] ==
            static_cast<std::uint8_t>(current_job)) {
            ++job_level;
        }
    }

    const std::int32_t growth_group =
        kJobGrowthMap[
            static_cast<std::size_t>(current_job)];
    const TableData* growth =
        growth_group >= 0
            ? tables.find(
                  901 + growth_group * 2 - gender())
            : nullptr;
    const std::int32_t column = job_level - 1;
    if (growth && column >= 0 &&
        growth->columnCount() > column) {
        for (std::size_t row = 0;
             row < kInitialParameterOffsets.size();
             ++row) {
            const std::size_t offset =
                kInitialParameterOffsets[row];
            writeI32(
                offset,
                retailAdd(
                    readI32(offset),
                    growth->value(
                        static_cast<std::int32_t>(row),
                        column)));
        }
    }

    if (new_level == 5) {
        writeI32(0x1c, 6);
        writeI32(0x20, 6);
    }
    if (new_level >= 5) {
        writeI32(kTotalKillCountOffset, 0);
        for (std::size_t kind = 0;
             kind < kKillCountCount;
             ++kind) {
            writeI32(kKillCountOffset + kind * 4u, 0);
        }
    }
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
    setCurrentLife(value, baseMaximumLife());
}

void PlayerData::setCurrentLife(
    std::int32_t value,
    std::int32_t maximum_life) {
    writeI32(
        0x34,
        std::clamp(
            value,
            0,
            std::max(0, maximum_life)));
}

void PlayerData::setCurrentMana(std::int32_t value) {
    setCurrentMana(value, baseMaximumMana());
}

void PlayerData::setCurrentMana(
    std::int32_t value,
    std::int32_t maximum_mana) {
    writeI32(
        0x3c,
        std::clamp(
            value,
            0,
            std::max(0, maximum_mana)));
}

void PlayerData::restoreForRespawn() {
    // FUN_00440c20 restores both pools when a scenario transition carries
    // the retail revive flag.
    setCurrentLife(baseMaximumLife());
    setCurrentMana(baseMaximumMana());
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

std::int32_t PlayerData::companionLevel() const {
    return readI32(0x144);
}

std::int32_t PlayerData::companionExperience() const {
    return readI32(0x148);
}

std::int32_t PlayerData::companionRespawnCounter() const {
    return readI32(0x14c);
}

std::size_t PlayerData::companionCount() const {
    return companion_levels_.size();
}

std::int32_t PlayerData::companionLevel(
    std::int32_t type) const {
    return type >= 0 &&
                   static_cast<std::size_t>(type) <
                       companion_levels_.size()
        ? companion_levels_[static_cast<std::size_t>(type)]
        : 0;
}

std::int32_t PlayerData::companionExperience(
    std::int32_t type) const {
    return type >= 0 &&
                   static_cast<std::size_t>(type) <
                       companion_experiences_.size()
        ? companion_experiences_[static_cast<std::size_t>(type)]
        : 0;
}

const std::vector<std::int32_t>&
PlayerData::companionLevels() const {
    return companion_levels_;
}

const std::vector<std::int32_t>&
PlayerData::companionExperiences() const {
    return companion_experiences_;
}

bool PlayerData::restoreCompanionProgress(
    std::vector<std::int32_t> levels,
    std::vector<std::int32_t> experiences) {
    if (levels.empty() ||
        levels.size() != experiences.size() ||
        (!companion_levels_.empty() &&
         levels.size() != companion_levels_.size()) ||
        companionType() < 0 ||
        static_cast<std::size_t>(companionType()) >=
            levels.size()) {
        return false;
    }
    companion_levels_ = std::move(levels);
    companion_experiences_ = std::move(experiences);
    writeI32(
        0x144,
        companion_levels_[
            static_cast<std::size_t>(companionType())]);
    writeI32(
        0x148,
        companion_experiences_[
            static_cast<std::size_t>(companionType())]);
    return true;
}

bool PlayerData::switchCompanion(std::int32_t type) {
    if (type < 0 ||
        static_cast<std::size_t>(type) >=
            companion_levels_.size() ||
        companionType() < 0 ||
        static_cast<std::size_t>(companionType()) >=
            companion_levels_.size()) {
        return false;
    }
    storeActiveCompanionProgress();
    writeI32(0x140, type);
    writeI32(
        0x144,
        companion_levels_[static_cast<std::size_t>(type)]);
    writeI32(
        0x148,
        companion_experiences_[static_cast<std::size_t>(type)]);
    writeI32(0x14c, 0);
    return true;
}

void PlayerData::setCompanionRespawnCounter(
    std::int32_t value) {
    writeI32(0x14c, std::max<std::int32_t>(value, 0));
}

void PlayerData::awardCompanionKillExperience(
    std::int32_t source_character_number,
    std::int32_t local_player_slot,
    bool companion_alive) {
    const std::int32_t level_cap =
        std::min<std::int32_t>(
            level() / 3 + 2, 35);
    std::int32_t companion_experience =
        companionExperience();
    if (source_character_number >= 0 &&
        source_character_number % 10 ==
            local_player_slot) {
        if (companion_alive &&
            companionLevel() < level_cap) {
            companion_experience =
                retailAdd(companion_experience, 1);
        } else {
            companion_experience = 0;
        }
    }
    writeI32(0x148, companion_experience);
    storeActiveCompanionProgress();
}

bool PlayerData::applyCompanionLevelThreshold(
    const TableDatabase& tables) {
    const std::int32_t level_cap =
        std::min<std::int32_t>(
            level() / 3 + 2, 35);
    std::int32_t companion_level =
        companionLevel();
    std::int32_t companion_experience =
        companionExperience();
    const TableData* progression =
        tables.find(800 + companionType());
    bool level_gained = false;
    while (progression &&
           companion_level >= 1 &&
           progression->contains(
               18, companion_level - 1)) {
        const std::int32_t threshold =
            progression->value(
                18, companion_level - 1);
        if (threshold > companion_experience) {
            break;
        }
        if (companion_level >= level_cap) {
            companion_experience = 0;
            break;
        }
        companion_experience =
            retailSubtract(
                companion_experience, threshold);
        ++companion_level;
        level_gained = true;
    }
    if (companion_level == level_cap) {
        companion_experience = 0;
    }
    writeI32(0x144, companion_level);
    writeI32(0x148, companion_experience);
    storeActiveCompanionProgress();
    return level_gained;
}

bool PlayerData::initializeCompanionProgress(
    const TableDatabase& tables,
    std::string* error) {
    const TableData* catalog =
        tables.find(kCompanionCatalogTable);
    if (!catalog || catalog->rowCount() <= 0 ||
        companionType() < 0 ||
        companionType() >= catalog->rowCount()) {
        setError(
            error,
            "The companion catalog does not match the player record.");
        return false;
    }
    companion_levels_.assign(
        static_cast<std::size_t>(catalog->rowCount()), 1);
    companion_experiences_.assign(
        static_cast<std::size_t>(catalog->rowCount()), 0);
    storeActiveCompanionProgress();
    return true;
}

void PlayerData::storeActiveCompanionProgress() {
    const std::int32_t type = companionType();
    if (type < 0 ||
        static_cast<std::size_t>(type) >=
            companion_levels_.size() ||
        companion_levels_.size() !=
            companion_experiences_.size()) {
        return;
    }
    companion_levels_[static_cast<std::size_t>(type)] =
        companionLevel();
    companion_experiences_[static_cast<std::size_t>(type)] =
        companionExperience();
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

std::int32_t PlayerData::basePhysicalAttack() const {
    return initialParameter(5);
}

std::int32_t PlayerData::basePhysicalDefense() const {
    return initialParameter(6);
}

std::int32_t PlayerData::baseMagicalAttack() const {
    return initialParameter(7);
}

std::int32_t PlayerData::baseMagicalDefense() const {
    return initialParameter(8);
}

std::int32_t PlayerData::baseHitRate() const {
    return initialParameter(9);
}

std::int32_t PlayerData::baseEvasionRate() const {
    return initialParameter(10);
}

std::int32_t PlayerData::baseMagicalHitRate() const {
    return initialParameter(11);
}

std::int32_t PlayerData::baseMagicalEvasionRate() const {
    return initialParameter(12);
}

std::int32_t PlayerData::elementX() const {
    return readI32(0x64);
}

std::int32_t PlayerData::elementY() const {
    return readI32(0x68);
}

bool PlayerData::clearElementCondition() {
    if (elementX() == 0 && elementY() == 0) {
        return false;
    }
    writeI32(0x64, 0);
    writeI32(0x68, 0);
    return true;
}

bool PlayerData::applyElementMedicine(
    std::int32_t element,
    std::int32_t distance) {
    const ElementAnchor before{elementX(), elementY()};
    const ElementAnchor after = moveRetailElementCondition(
        before, element, distance);
    if (after.x == before.x && after.y == before.y) {
        return false;
    }
    writeI32(0x64, after.x);
    writeI32(0x68, after.y);
    return true;
}

std::array<std::int32_t, 17>
PlayerData::combatPacketStateWords() const {
    std::array<std::int32_t, 17> words{};
    for (std::size_t index = 0;
         index < words.size();
         ++index) {
        words[index] = readI32(0x6c + index * 4);
    }
    return words;
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
