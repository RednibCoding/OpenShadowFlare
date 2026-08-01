#include "player_spell_parameters.hpp"

#include "enemy_effect_impact.hpp"
#include "items/item_database.hpp"
#include "items/player_equipment.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "player_magic.hpp"

#include <algorithm>

namespace osf {

PlayerSpellParameters playerSpellParameters(
    const PlayerMagic& magic,
    std::int32_t spell,
    const PlayerEquipment& equipment,
    const ItemDatabase& items,
    const TableDatabase& tables,
    std::int32_t magic_level_modifier,
    bool increased_power) {
    PlayerSpellParameters result;
    if (spell < 0 ||
        spell >=
            static_cast<std::int32_t>(
                PlayerMagic::spell_count)) {
        return result;
    }

    std::int32_t level = magic.level(spell);
    if (increased_power) {
        level += 2;
    }
    level = std::clamp(level, std::int32_t{1}, std::int32_t{20});
    result.effective_level = std::clamp(
        level + magic_level_modifier, std::int32_t{1}, std::int32_t{30});

    const std::int32_t base_mana =
        retailEffectParameter(
            tables,
            spell,
            result.effective_level,
            2);
    result.mana_cost =
        base_mana < 0
            ? 0
            : std::max(
                  base_mana -
                      equipment.instanceParameterBonus(
                          19, items),
                  std::int32_t{1});
    result.effect_value = std::max(
        retailEffectParameter(
            tables,
            spell,
            result.effective_level,
            0),
        std::int32_t{0});

    result.maximum_level = magic.level(spell) == 20;
    if (!result.maximum_level) {
        const TableData* experience = tables.find(27);
        const std::int32_t column =
            std::clamp(magic.level(spell), std::int32_t{1}, std::int32_t{20}) - 1;
        if (experience &&
            experience->contains(spell, column)) {
            result.experience_threshold =
                experience->value(spell, column);
        }
    }
    return result;
}

}  // namespace osf
