#include "scenario_attached_effect_command.hpp"

#include <cstddef>

namespace osf {
namespace {

constexpr std::size_t kArgumentCount = 2;
constexpr std::int32_t kPlayerOwner = 1;
constexpr std::int32_t kScenarioActorOwner = 4;

}  // namespace

bool makeScenarioAttachedEffectRequest(
    const std::vector<std::int32_t>& arguments,
    std::int32_t owner_kind,
    const ObjectBounds& source_judgement,
    CombatEffectSpawnRequest& request) {
    if (arguments.size() != kArgumentCount ||
        (owner_kind != kPlayerOwner &&
         owner_kind != kScenarioActorOwner)) {
        return false;
    }

    request = {};
    request.valid = true;
    request.effect_number = arguments[0];
    request.owner_kind = owner_kind;
    request.source_character_number = arguments[1];
    request.target_kind = 0;
    request.target_identifier = 0;
    request.constructor_value_6 = 0;
    request.constructor_value_7 = 0;
    request.direction_radians = 0.0;
    request.has_explicit_origin = false;
    request.has_source_judgement = true;
    request.source_judgement = source_judgement;
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
    return true;
}

}  // namespace osf
