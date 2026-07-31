#ifndef OPENSHADOWFLARE_PLAYER_RUNTIME_PROFILE_HPP
#define OPENSHADOWFLARE_PLAYER_RUNTIME_PROFILE_HPP

#include <cstdint>

namespace osf {

class ItemDatabase;
class PlayerData;
class PlayerEquipment;
class PlayerSustainedSpell;
class TableDatabase;

struct PlayerRuntimeProfile {
    std::int32_t attack_speed_raw = 0;
    std::int32_t walking_speed_raw = 0;
    std::int32_t maximum_life = 1;
    std::int32_t maximum_mana = 1;
    std::int32_t weight_capacity = 0;
    std::int32_t physical_attack = 1;
    std::int32_t physical_defense = 1;
    std::int32_t hit_rate = 1;
    std::int32_t physical_evasion = 1;
    std::int32_t magical_attack = 1;
    std::int32_t magical_defense = 1;
    std::int32_t magical_hit_rate = 1;
    std::int32_t magical_evasion = 1;

    std::int32_t walkingSpeedTier() const;
};

PlayerRuntimeProfile buildPlayerRuntimeProfile(
    const PlayerData& player,
    const PlayerEquipment& equipment,
    const ItemDatabase& items,
    const PlayerSustainedSpell& berserker,
    const TableDatabase& tables);

}  // namespace osf

#endif
