#include "player_experience_award.hpp"

#include "core/retail_integer.hpp"
#include "player_data.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>

namespace osf {
namespace {

struct LevelUpField {
    std::size_t parameter_row = 0;
    const char* prefix = nullptr;
};

constexpr std::array<LevelUpField, 13> kLevelUpFields{{
    {2, "  HP                    +"},
    {3, "  MP                    +"},
    {0, "  Attack Speed          +"},
    {1, "  Walking Speed         +"},
    {4, "  Strength              +"},
    {5, "  Attack                +"},
    {6, "  Defense               +"},
    {9, "  Hit Rate              +"},
    {10, "  Evasion Rate          +"},
    {7, "  Magical Attack        +"},
    {8, "  Magical Defense       +"},
    {11, "  Magical Hit Rate      +"},
    {12, "  Magical Evasion Rate  +"},
}};

std::array<std::int32_t, 13> playerParameters(
    const PlayerData& player) {
    std::array<std::int32_t, 13> result{};
    for (std::size_t row = 0; row < result.size(); ++row) {
        result[row] = player.initialParameter(row);
    }
    return result;
}

}  // namespace

PlayerLevelUpResult applyRetailPlayerLevelThreshold(
    PlayerData& player,
    const TableDatabase& tables) {
    PlayerLevelUpResult result;
    const std::array<std::int32_t, 13> parameters_before =
        playerParameters(player);
    result.level_gained = player.applyLevelThreshold(tables);
    if (!result.level_gained) {
        return result;
    }

    std::ostringstream text;
    text << "Level " << player.level() << '\n';
    const std::array<std::int32_t, 13> parameters_after =
        playerParameters(player);
    for (const LevelUpField& field : kLevelUpFields) {
        const std::int32_t change =
            parameters_after[field.parameter_row] -
            parameters_before[field.parameter_row];
        if (change == 0) {
            continue;
        }
        text << field.prefix
             << std::setw(4) << change
             << "    \n";
    }
    result.notice = text.str();
    result.notice_counter = 900;
    if (player.level() == 5) {
        result.audio_samples.push_back(64);
    }
    result.audio_samples.push_back(63);
    return result;
}

PlayerExperienceAwardResult awardRetailPlayerExperiencePercentage(
    PlayerData& player,
    std::int32_t percentage,
    const TableDatabase& tables) {
    PlayerExperienceAwardResult result;
    if (player.level() <= 0 || player.level() >= 100) {
        return result;
    }
    const std::int32_t threshold =
        player.experienceThreshold(tables);
    if (threshold <= 0) {
        return result;
    }

    const std::int64_t quotient =
        static_cast<std::int64_t>(threshold) * percentage /
        100;
    result.experience_awarded = retailSignedWord(
        static_cast<std::uint32_t>(quotient));
    player.addExperience(result.experience_awarded);
    result.level_up =
        applyRetailPlayerLevelThreshold(player, tables);
    return result;
}

}  // namespace osf
