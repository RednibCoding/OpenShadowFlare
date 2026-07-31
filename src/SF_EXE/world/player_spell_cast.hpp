#ifndef OPENSHADOWFLARE_PLAYER_SPELL_CAST_HPP
#define OPENSHADOWFLARE_PLAYER_SPELL_CAST_HPP

#include "combat_effect_request.hpp"
#include "player_spell_parameters.hpp"

#include <array>
#include <cstdint>

namespace osf {

class TableDatabase;
class RetailRandom;

struct PlayerSpellCastStats {
    std::int32_t source_character_number = -1;
    std::int32_t player_level = 1;
    std::int32_t magical_attack = 0;
    std::int32_t physical_defense = 0;
    std::int32_t magical_defense = 0;
    std::int32_t magical_hit_rate = 0;
    std::array<std::int32_t, 8> element_affinities{};
    std::array<std::int32_t, 17> state_words{};
};

struct PlayerSpellCastInput {
    PlayerSpellCastStats stats;
    PlayerSpellParameters parameters;
    std::int32_t target_character_number = -1;
    WorldPosition source_position;
    ObjectBounds source_judgement;
    WorldPosition target_position;
    std::int32_t effect_delay = 0;
};

bool playerSpellRequiresCharacterTarget(
    std::int32_t spell);

CombatEffectSpawnRequest buildPlayerSpellCast(
    std::int32_t spell,
    const PlayerSpellCastInput& input,
    const TableDatabase& tables,
    RetailRandom* random = nullptr);

}  // namespace osf

#endif
