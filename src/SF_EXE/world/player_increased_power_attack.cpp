#include "player_increased_power_attack.hpp"

#include "core/retail_random.hpp"

#include <utility>

namespace osf {

PlayerIncreasedPowerAttackResult
resolvePlayerIncreasedPowerAttack(
    const PlayerIncreasedPowerAttackInput& input,
    RetailRandom& random) {
    PlayerIncreasedPowerAttackResult result;
    if (input.source_character_number < 0 ||
        input.target_identifiers.empty()) {
        return result;
    }

    CombatPacket packet =
        buildPlayerAttackPacket(input.stats, 20006, random);
    result.projectiles.reserve(
        input.target_identifiers.size());
    for (const std::int32_t target :
         input.target_identifiers) {
        if (target < 0) {
            continue;
        }
        CombatEffectSpawnRequest request;
        request.valid = true;
        request.effect_number = 9000;
        request.owner_kind = 1;
        request.source_character_number =
            input.source_character_number;
        request.target_kind = 20;
        request.target_identifier = target;
        request.constructor_value_6 = 400;
        request.constructor_value_7 = 350;
        request.direction_radians = 0.0;
        request.has_packet = true;
        request.packet = packet;
        request.packet_kind = 8;
        request.instance_identifier = -1;
        request.constructor_value_21 = 200;
        request.constructor_value_22 = 0;
        result.projectiles.push_back(
            std::move(request));
    }
    result.valid = !result.projectiles.empty();
    result.consume_durability = result.valid;
    return result;
}

}  // namespace osf
