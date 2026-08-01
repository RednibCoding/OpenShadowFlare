#ifndef OPENSHADOWFLARE_PLAYER_EXPERIENCE_AWARD_HPP
#define OPENSHADOWFLARE_PLAYER_EXPERIENCE_AWARD_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace osf {

class PlayerData;
class TableDatabase;

struct PlayerLevelUpResult {
    bool level_gained = false;
    std::string notice;
    std::int32_t notice_counter = 0;
    std::vector<std::int32_t> audio_samples;
};

struct PlayerExperienceAwardResult {
    std::int32_t experience_awarded = 0;
    PlayerLevelUpResult level_up;
};

PlayerLevelUpResult applyRetailPlayerLevelThreshold(
    PlayerData& player,
    const TableDatabase& tables);

PlayerExperienceAwardResult awardRetailPlayerExperiencePercentage(
    PlayerData& player,
    std::int32_t percentage,
    const TableDatabase& tables);

}  // namespace osf

#endif
