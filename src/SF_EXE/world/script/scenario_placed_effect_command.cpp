#include "scenario_placed_effect_command.hpp"

#include <cstddef>

namespace osf {
namespace {

constexpr std::size_t kArgumentCount = 7;
constexpr std::int32_t kDefaultDirection = 8;

}  // namespace

bool makeScenarioPlacedEffectRequest(
    const std::vector<std::int32_t>& arguments,
    CombatEffectSpawnRequest& request) {
    if (arguments.size() != kArgumentCount) {
        return false;
    }

    request = {};
    request.valid = true;
    request.effect_number = arguments[0];
    request.owner_kind = 0;
    request.source_character_number = 0;
    request.target_kind = 0;
    request.target_identifier = 0;
    request.constructor_value_6 = 0;
    request.constructor_value_7 = arguments[3];
    request.direction_radians = 0.0;
    request.has_explicit_origin = true;
    request.origin = {arguments[1], arguments[2]};
    request.has_source_judgement = true;
    request.source_judgement = {
        0,
        0,
        arguments[5],
        arguments[6],
    };
    request.constructor_value_12 = 0;
    request.has_packet = false;
    request.packet_kind = arguments[4] < 0
        ? kDefaultDirection
        : arguments[4];
    request.instance_identifier = -1;
    request.constructor_value_16 = 0;
    request.constructor_value_17 = 0;
    request.constructor_value_18 = 0;
    request.constructor_value_19 = 0;
    request.constructor_value_20 = 0;
    request.constructor_value_21 = 200;
    request.constructor_value_22 = 0;
    return true;
}

}  // namespace osf
