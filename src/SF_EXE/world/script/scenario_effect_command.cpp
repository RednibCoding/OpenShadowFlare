#include "scenario_effect_command.hpp"

#include "core/retail_integer.hpp"
#include "world/actor_direction.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace osf {
namespace {

constexpr std::size_t kArgumentCount = 14;
constexpr std::int32_t kEffectTargetKind = 19;
constexpr std::int32_t kDefaultPacketDirection = 8;
constexpr std::int32_t kImpactHitRating = 9999;

WorldPosition projectedOrigin(
    const std::vector<std::int32_t>& arguments,
    double direction_radians) {
    const std::int32_t distance = arguments[7];
    return {
        retailAdd(
            arguments[0],
            static_cast<std::int32_t>(
                std::cos(direction_radians) * distance)),
        retailSubtract(
            arguments[1],
            static_cast<std::int32_t>(
                std::sin(direction_radians) * distance)),
    };
}

CombatPacket makePacket(
    const std::vector<std::int32_t>& arguments,
    std::int32_t random_value) {
    CombatPacket packet;
    // The opcode-30 handler at 0x0043309b writes this exact subset before
    // passing the complete packet to the shared effect-request owner.
    constexpr std::size_t zero_words[] = {
        1, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
        31, 38, 39, 42, 44, 76,
    };
    for (std::size_t word : zero_words) {
        packet.write(word, 0);
    }
    packet.write(0, 2);
    packet.write(2, -1);
    packet.write(4, arguments[5]);
    packet.write(32, -1);
    packet.write(
        34,
        arguments[2] != 0
            ? 21000 + random_value % 4
            : 21007 + random_value % 3);
    packet.write(35, kDefaultPacketDirection);
    packet.write(36, kImpactHitRating);
    packet.write(37, arguments[9] != 0 ? 1 : 0);
    packet.write(40, arguments[12]);
    packet.write(41, arguments[10]);
    packet.write(43, arguments[11]);
    packet.write(72, arguments[13]);
    packet.write(73, -1);
    packet.write(74, -1);
    packet.write(75, kDefaultPacketDirection);
    return packet;
}

}  // namespace

bool makeScenarioEffectRequest(
    const std::vector<std::int32_t>& arguments,
    std::int32_t random_value,
    CombatEffectSpawnRequest& request) {
    if (arguments.size() != kArgumentCount ||
        random_value < 0) {
        return false;
    }

    const double direction_radians =
        static_cast<double>(arguments[3]) *
        kRetailRadiansPerDegree;
    request = {};
    request.valid = true;
    request.effect_number = arguments[2];
    request.owner_kind = 0;
    request.source_character_number = -1;
    request.target_kind = kEffectTargetKind;
    request.target_identifier = -1;
    request.constructor_value_6 = arguments[4];
    request.constructor_value_7 = arguments[6];
    request.direction_radians = direction_radians;
    request.has_explicit_origin = true;
    request.origin = projectedOrigin(
        arguments, direction_radians);
    request.constructor_value_12 = 0;
    request.has_packet = true;
    request.packet = makePacket(arguments, random_value);
    request.packet_kind = arguments[8] < 0
        ? kDefaultPacketDirection
        : arguments[8];
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
