#ifndef OPENSHADOWFLARE_PLAYER_DATA_HPP
#define OPENSHADOWFLARE_PLAYER_DATA_HPP

#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "player_job.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace osf {

enum class PlayerDataSource {
    new_character,
    retail_save,
};

// This is the value stored by the retail executable in the player record.
enum class PlayerGender : std::int32_t {
    female = 0,
    male = 1,
};

constexpr std::int32_t playerGenderValue(PlayerGender gender) {
    return static_cast<std::int32_t>(gender);
}

struct PlayerLoadRequest {
    PlayerDataSource source = PlayerDataSource::new_character;
    std::string name;
    std::int32_t gender = playerGenderValue(PlayerGender::female);
    std::filesystem::path save_path;
};

class PlayerData {
public:
    static constexpr std::size_t retail_record_size = 0x160;
    static constexpr std::size_t initial_parameter_count = 13;

    bool initializeNew(
        std::string_view name,
        std::int32_t gender,
        const TableDatabase& tables,
        std::string* error = nullptr);
    bool loadRetailSave(
        const std::filesystem::path& path,
        std::string* error = nullptr);
    bool load(
        const PlayerLoadRequest& request,
        const TableDatabase& tables,
        std::string* error = nullptr);
    void clear();

    bool valid() const;
    std::string name() const;
    std::int32_t gender() const;
    std::int32_t job() const;
    void setJob(PlayerJob job);
    std::int32_t level() const;
    std::int32_t experience() const;
    std::int32_t experienceThreshold(
        const TableDatabase& tables) const;
    std::int32_t totalKillCount() const;
    std::int32_t killCount(std::size_t kind) const;
    std::int32_t jobLevel(std::int32_t job) const;
    void addExperience(std::int32_t amount);
    void addKillCount(std::size_t kind);
    bool applyLevelThreshold(const TableDatabase& tables);
    std::int32_t baseMaximumLife() const;
    std::int32_t currentLife() const;
    std::int32_t baseMaximumMana() const;
    std::int32_t currentMana() const;
    void setCurrentLife(std::int32_t value);
    void setCurrentLife(
        std::int32_t value,
        std::int32_t maximum_life);
    void setCurrentMana(std::int32_t value);
    void setCurrentMana(
        std::int32_t value,
        std::int32_t maximum_mana);
    void restoreForRespawn();
    bool restoreLife(
        std::int32_t amount,
        std::int32_t maximum_percent);
    bool restoreMana(
        std::int32_t amount,
        std::int32_t maximum_percent);
    std::int32_t initialParameter(std::size_t row) const;
    std::int32_t companionType() const;
    std::int32_t companionLevel() const;
    std::int32_t companionExperience() const;
    std::int32_t companionRespawnCounter() const;
    void setCompanionRespawnCounter(std::int32_t value);
    void awardCompanionKillExperience(
        std::int32_t source_character_number,
        std::int32_t local_player_slot,
        bool companion_alive);
    bool applyCompanionLevelThreshold(
        const TableDatabase& tables);
    std::int32_t baseAttackSpeed() const;
    std::int32_t baseWeightCapacity() const;
    std::int32_t basePhysicalAttack() const;
    std::int32_t basePhysicalDefense() const;
    std::int32_t baseMagicalAttack() const;
    std::int32_t baseMagicalDefense() const;
    std::int32_t baseHitRate() const;
    std::int32_t baseEvasionRate() const;
    std::int32_t baseMagicalHitRate() const;
    std::int32_t baseMagicalEvasionRate() const;
    std::int32_t elementX() const;
    std::int32_t elementY() const;
    bool clearElementCondition();
    bool applyElementMedicine(
        std::int32_t element,
        std::int32_t distance);
    std::array<std::int32_t, 17>
        combatPacketStateWords() const;
    std::int32_t walkingSpeedTier() const;
    const std::array<std::uint8_t, retail_record_size>&
        retailRecord() const;

private:
    std::int32_t readI32(std::size_t offset) const;
    void writeI32(std::size_t offset, std::int32_t value);
    void levelUp(const TableDatabase& tables);

    std::array<std::uint8_t, retail_record_size> record_{};
    bool valid_ = false;
};

}  // namespace osf

#endif
