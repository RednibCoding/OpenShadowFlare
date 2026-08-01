#ifndef OPENSHADOWFLARE_PLAYER_SPELL_PARAMETERS_HPP
#define OPENSHADOWFLARE_PLAYER_SPELL_PARAMETERS_HPP

#include <cstdint>

namespace osf {

class ItemDatabase;
class PlayerEquipment;
class PlayerMagic;
class TableDatabase;

struct PlayerSpellParameters {
    std::int32_t effective_level = 1;
    std::int32_t mana_cost = 0;
    std::int32_t effect_value = 0;
    std::int32_t experience_threshold = 0;
    bool maximum_level = false;
};

PlayerSpellParameters playerSpellParameters(
    const PlayerMagic& magic,
    std::int32_t spell,
    const PlayerEquipment& equipment,
    const ItemDatabase& items,
    const TableDatabase& tables,
    std::int32_t magic_level_modifier = 0,
    bool increased_power = false);

}  // namespace osf

#endif
