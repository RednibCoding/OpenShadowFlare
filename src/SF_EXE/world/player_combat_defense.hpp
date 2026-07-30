#ifndef OPENSHADOWFLARE_PLAYER_COMBAT_DEFENSE_HPP
#define OPENSHADOWFLARE_PLAYER_COMBAT_DEFENSE_HPP

#include "combat_damage.hpp"

#include <array>
#include <cstdint>

namespace osf {

class ItemDatabase;
class PlayerEquipment;
class PlayerInventory;

struct PlayerCombatDefenseSnapshot {
    std::int32_t character_number = -1;
    // These are the live derived values after equipment, affinity, and
    // status refresh, not the corresponding base player-record fields.
    std::int32_t attack = 0;
    std::int32_t physical_defense = 0;
    std::int32_t magical_defense = 0;
    std::int32_t element_x = 0;
    std::int32_t element_y = 0;
};

std::array<std::int32_t, 8> buildPlayerElementAffinities(
    const PlayerCombatDefenseSnapshot& snapshot,
    const PlayerEquipment& equipment,
    const PlayerInventory& inventory,
    const ItemDatabase& item_database);

CombatDefense buildPlayerCombatDefense(
    const PlayerCombatDefenseSnapshot& snapshot,
    const PlayerEquipment& equipment,
    const PlayerInventory& inventory,
    const ItemDatabase& item_database);

}  // namespace osf

#endif
