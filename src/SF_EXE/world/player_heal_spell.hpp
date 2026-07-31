#ifndef OPENSHADOWFLARE_PLAYER_HEAL_SPELL_HPP
#define OPENSHADOWFLARE_PLAYER_HEAL_SPELL_HPP

#include "combat_effect_request.hpp"

#include <cstdint>

namespace osf {

struct PlayerHealSpellInput {
    std::int32_t source_character_number = -1;
    std::int32_t current_life = 0;
    std::int32_t maximum_life = 0;
    std::int32_t heal_percent = 0;
    ObjectBounds source_judgement;
};

struct PlayerHealSpellResolution {
    bool valid = false;
    std::int32_t restored_life = 0;
    std::int32_t healed_amount = 0;
    bool award_practice = false;
    std::int32_t audio_sample = -1;
    CombatEffectSpawnRequest visual;
};

PlayerHealSpellResolution resolvePlayerHealSpell(
    const PlayerHealSpellInput& input);

}  // namespace osf

#endif
