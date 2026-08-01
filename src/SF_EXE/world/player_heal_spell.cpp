#include "player_heal_spell.hpp"

#include "core/retail_integer.hpp"

#include <algorithm>
#include <cstdint>

namespace osf {
namespace {

constexpr std::int32_t kHealVisualEffect = 21020;
constexpr std::int32_t kHealAudioSample = 17;

CombatEffectSpawnRequest healVisual(
    const PlayerHealSpellInput& input) {
    CombatEffectSpawnRequest request;
    request.valid = true;
    request.effect_number = kHealVisualEffect;
    request.owner_kind = 1;
    request.source_character_number =
        input.source_character_number;
    request.target_kind = 0;
    request.target_identifier = 0;
    request.constructor_value_6 = 0;
    request.constructor_value_7 = 0;
    request.direction_radians = 0.0;
    request.has_source_judgement = true;
    request.source_judgement = input.source_judgement;
    request.constructor_value_12 = 0;
    request.has_packet = false;
    request.packet_kind = 8;
    request.instance_identifier = -1;
    request.constructor_value_16 = 0;
    request.constructor_value_17 = 0;
    request.constructor_value_18 = 0;
    request.constructor_value_19 = 0;
    request.constructor_value_20 = 0;
    request.constructor_value_21 = 200;
    request.constructor_value_22 = 0;
    return request;
}

}  // namespace

PlayerHealSpellResolution resolvePlayerHealSpell(
    const PlayerHealSpellInput& input) {
    PlayerHealSpellResolution result;
    if (input.source_character_number < 0 ||
        input.maximum_life < 0 ||
        input.current_life < 0 ||
        input.current_life > input.maximum_life) {
        return result;
    }

    result.valid = true;
    result.restored_life = input.current_life;
    result.visual = healVisual(input);
    if (input.current_life == input.maximum_life) {
        return result;
    }

    const std::int32_t authored_amount =
        retailMultiply(
            input.heal_percent,
            input.maximum_life) /
        100;
    result.healed_amount =
        std::min(
            authored_amount,
            input.maximum_life - input.current_life);
    result.restored_life =
        retailAdd(
            input.current_life,
            result.healed_amount);
    result.award_practice = true;
    result.audio_sample = kHealAudioSample;
    return result;
}

}  // namespace osf
