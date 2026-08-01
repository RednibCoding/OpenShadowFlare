#include "player_ranged_attack.hpp"

#include "actor_direction.hpp"
#include "core/retail_integer.hpp"
#include "core/retail_random.hpp"
#include "items/item_database.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace osf {
namespace {

constexpr std::int32_t kRangedJob = 5;
constexpr std::int32_t kEnemyAndObjectTargetMask = 20;
constexpr std::int32_t kProjectileDisplayHeight = 350;
constexpr std::int32_t kProjectileOriginDistance = 200;
constexpr std::int32_t kStraightDirection = 8;

constexpr std::array<std::int32_t, 4>
    kEffectNumbers{{1, 0, 4, 5}};

constexpr std::array<double, 7> kWideSpread{{
    0.0,
    -15.0 * kRetailRadiansPerDegree,
    15.0 * kRetailRadiansPerDegree,
    -30.0 * kRetailRadiansPerDegree,
    30.0 * kRetailRadiansPerDegree,
    -45.0 * kRetailRadiansPerDegree,
    45.0 * kRetailRadiansPerDegree,
}};

constexpr std::array<double, 7> kMediumSpread{{
    0.0,
    -10.0 * kRetailRadiansPerDegree,
    10.0 * kRetailRadiansPerDegree,
    -20.0 * kRetailRadiansPerDegree,
    20.0 * kRetailRadiansPerDegree,
    -30.0 * kRetailRadiansPerDegree,
    30.0 * kRetailRadiansPerDegree,
}};

constexpr std::array<double, 7> kNarrowSpread{{
    0.0,
    -8.0 * kRetailRadiansPerDegree,
    8.0 * kRetailRadiansPerDegree,
    -16.0 * kRetailRadiansPerDegree,
    16.0 * kRetailRadiansPerDegree,
    -24.0 * kRetailRadiansPerDegree,
    24.0 * kRetailRadiansPerDegree,
}};

WorldPosition projectedPosition(
    WorldPosition position,
    double angle,
    std::int32_t distance) {
    return {
        retailAdd(
            position.x,
            static_cast<std::int32_t>(
                std::cos(angle) * distance)),
        retailSubtract(
            position.y,
            static_cast<std::int32_t>(
                std::sin(angle) * distance)),
    };
}

std::int32_t hitEffect(
    std::int32_t selector,
    RetailRandom& random) {
    return selector == 0
        ? random.next() % 4 + 21000
        : random.next() % 3 + 21007;
}

struct Pattern {
    std::int32_t count = 0;
    bool homing = false;
    std::int32_t spread_group = 0;
    bool explicit_two_shot_origins = false;
};

Pattern patternFor(std::int32_t value) {
    switch (value) {
    case 0:
        return {1, false, 0, false};
    case 1:
        return {2, false, 2, true};
    case 2:
        return {1, true, 0, false};
    case 3:
        return {3, false, 0, false};
    case 4:
        return {3, true, 0, false};
    case 5:
        return {5, false, 1, false};
    case 6:
        return {5, true, 1, false};
    case 7:
        return {7, false, 2, false};
    case 8:
        return {7, true, 2, false};
    default:
        return {};
    }
}

const std::array<double, 7>& spreadFor(
    std::int32_t group) {
    if (group == 1) {
        return kMediumSpread;
    }
    if (group == 2) {
        return kNarrowSpread;
    }
    return kWideSpread;
}

}  // namespace

std::int32_t retailRangedPhysicalAttack(
    std::int32_t physical_attack,
    std::int32_t current_job,
    std::int32_t ranged_job_level) {
    if (current_job == kRangedJob) {
        return std::max(physical_attack, std::int32_t{1});
    }
    const std::int32_t percent =
        std::min(
            ranged_job_level * 50 / 30 + 40,
            std::int32_t{90});
    return std::max(
        retailMultiply(physical_attack, percent) / 100,
        std::int32_t{1});
}

PlayerRangedAttackResult resolvePlayerRangedAttack(
    const PlayerRangedAttackInput& input,
    RetailRandom& random) {
    PlayerRangedAttackResult result;
    if (!input.weapon ||
        input.weapon->category != 0 ||
        input.weapon->subtype != 5 ||
        input.source_character_number < 0 ||
        input.target_identifier < 0 ||
        input.weapon->ranged_effect_selector < 0 ||
        static_cast<std::size_t>(
            input.weapon->ranged_effect_selector) >=
            kEffectNumbers.size() ||
        input.weapon->ranged_travel_speed <= 0) {
        return result;
    }
    const Pattern pattern =
        patternFor(input.weapon->ranged_pattern);
    if (pattern.count <= 0) {
        return result;
    }

    PlayerAttackImpactStats stats = input.stats;
    stats.physical_attack =
        retailRangedPhysicalAttack(
            stats.physical_attack,
            input.current_job,
            input.ranged_job_level);
    CombatPacket packet =
        buildPlayerAttackPacket(stats, -1, random);
    packet.write(
        34,
        hitEffect(
            input.weapon->ranged_effect_selector,
            random));

    const double target_angle =
        retailAngleForVector(
            input.target_position.x -
                input.source_position.x,
            input.target_position.y -
                input.source_position.y);
    const std::int32_t packet_direction =
        retailDirectionForAngle(target_angle);
    const std::array<double, 7>& spread =
        spreadFor(pattern.spread_group);
    result.projectiles.reserve(
        static_cast<std::size_t>(pattern.count));
    for (std::int32_t index = 0;
         index < pattern.count;
         ++index) {
        const double direction =
            target_angle +
            (pattern.explicit_two_shot_origins
                 ? 0.0
                 : spread[static_cast<std::size_t>(index)]);
        CombatEffectSpawnRequest request;
        request.valid = true;
        request.effect_number =
            kEffectNumbers[
                static_cast<std::size_t>(
                    input.weapon->ranged_effect_selector)];
        request.owner_kind =
            pattern.explicit_two_shot_origins ? 0 : 1;
        request.source_character_number =
            input.source_character_number;
        request.target_kind =
            kEnemyAndObjectTargetMask;
        request.target_identifier =
            input.target_identifier;
        request.constructor_value_6 =
            input.weapon->ranged_travel_speed;
        request.constructor_value_7 =
            kProjectileDisplayHeight;
        request.direction_radians = direction;
        if (pattern.explicit_two_shot_origins) {
            request.has_explicit_origin = true;
            const double origin_direction =
                target_angle +
                (index == 0 ? -8.0 : 8.0) *
                    kRetailRadiansPerDegree;
            request.origin =
                projectedPosition(
                    input.source_position,
                    origin_direction,
                    kProjectileOriginDistance);
            request.constructor_value_21 = 0;
        } else {
            request.constructor_value_21 =
                kProjectileOriginDistance;
        }
        request.has_packet = true;
        request.packet = packet;
        request.packet_kind =
            pattern.homing ||
                    (input.weapon->ranged_pattern == 0)
                ? kStraightDirection
                : packet_direction;
        request.constructor_value_20 =
            pattern.homing ? 1 : 0;
        request.constructor_value_22 =
            input.weapon->ranged_pierces_targets
                ? 1
                : 0;
        result.projectiles.push_back(
            std::move(request));
    }
    result.valid = true;
    result.consume_durability = true;
    return result;
}

}  // namespace osf
