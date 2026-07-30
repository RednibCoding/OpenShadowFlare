#include "enemy_direct_impact.hpp"

#include "core/retail_random.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace osf {
namespace {

constexpr std::int32_t kMinimumHitChance = 20;
constexpr std::int32_t kMaximumHitChance = 98;
constexpr std::int32_t kHitAudioSample = 6;
constexpr std::int32_t kHitEvent = 17;
constexpr std::int32_t kDefaultTargetKind = 19;

bool validVariant(std::int32_t variant) {
    return variant >= 0 && variant < 3;
}

void buildCommonPacket(
    CombatPacket& packet,
    const EnemyDirectImpactInput& input,
    RetailRandom& random,
    bool special_effect) {
    const std::size_t variant =
        static_cast<std::size_t>(input.variant);
    const EnemyPresentationProfile& profile =
        *input.profile;
    packet.write(0, 2);
    packet.write(1, 0);
    packet.write(2, input.source_character_number);
    packet.write(3, 0);
    packet.write(4, profile.direct_packet_word_4[variant]);
    packet.write(31, profile.packet_word_31);
    packet.write(32, profile.direct_packet_word_32);
    packet.write(34, random.next() % 4 + 21000);
    packet.write(35, 8);
    packet.write(36, profile.direct_hit_rate[variant]);
    packet.write(37, 0);
    packet.write(38, special_effect ? 0 : 1);
    packet.write(39, 0);
    packet.write(40, profile.direct_packet_word_40[variant]);
    packet.write(41, profile.direct_packet_word_41[variant]);
    packet.write(42, 0);
    packet.write(43, profile.direct_packet_word_43[variant]);
    packet.write(44, 0);
    packet.write(72, 1);
    packet.write(73, -1);
    packet.write(74, -1);
    packet.write(75, 8);
    packet.write(76, 0);
}

EnemyEffectSpawnRequest buildSpecialEffect(
    const EnemyDirectImpactInput& input,
    CombatPacket packet,
    RetailRandom& random) {
    const EnemyPresentationProfile& profile =
        *input.profile;
    const std::int32_t effect_number =
        profile.direct_special_effect_number;
    const std::int32_t visual_draw = random.next();
    std::int32_t packet_mode = 0;
    std::int32_t visual_number = 0;
    std::int32_t secondary_visual = -1;
    switch (effect_number) {
    case 0:
        visual_number = visual_draw % 3 + 21007;
        break;
    case 4:
        visual_number = visual_draw % 3 + 21007;
        secondary_visual = 20000;
        break;
    case 5:
        visual_number = visual_draw % 3 + 21007;
        packet_mode = 1;
        secondary_visual = 21013;
        break;
    case 7:
        visual_number = visual_draw % 3 + 21007;
        packet_mode = 2;
        break;
    default:
        visual_number = visual_draw % 4 + 21000;
        break;
    }
    packet.write(3, packet_mode);
    packet.write(34, visual_number);
    packet.write(74, secondary_visual);

    EnemyEffectSpawnRequest request;
    request.valid = true;
    request.effect_number = effect_number;
    request.source_character_number =
        input.source_character_number;
    request.target_kind = kDefaultTargetKind;
    request.target_identifier = -1;
    request.constructor_value_6 =
        profile.direct_special_constructor_value_6;
    request.constructor_value_7 =
        profile.direct_special_constructor_value_7;
    request.direction_radians = input.direction_radians;
    request.has_explicit_origin = false;
    request.has_source_judgement = false;
    request.constructor_value_12 = 0;
    request.packet = packet;
    request.packet_kind = 8;
    request.instance_identifier = -1;
    request.constructor_value_16 = 0;
    request.constructor_value_17 = 0;
    request.constructor_value_18 = 0;
    request.constructor_value_19 = 0;
    request.constructor_value_20 = 0;
    request.constructor_value_21 =
        profile.direct_special_constructor_value_21;
    request.constructor_value_22 = 0;
    return request;
}

}  // namespace

bool enemyDirectImpactUsesSpecialEffect(
    const EnemyPresentationProfile& profile,
    std::int32_t variant) {
    return validVariant(variant) &&
           profile.direct_special_effect_number != -1 &&
           profile.direct_special_variant == variant;
}

std::int32_t retailEnemyHitChance(
    std::int32_t attack_value,
    std::int32_t defense_value) {
    return std::clamp(
        attack_value - defense_value,
        kMinimumHitChance,
        kMaximumHitChance);
}

EnemyDirectImpactResult resolveEnemyDirectImpact(
    const EnemyDirectImpactInput& input,
    RetailRandom& random) {
    EnemyDirectImpactResult result;
    if (!input.profile ||
        !validVariant(input.variant)) {
        return result;
    }
    result.valid = true;
    result.special_effect =
        enemyDirectImpactUsesSpecialEffect(
            *input.profile, input.variant);
    buildCommonPacket(
        result.packet,
        input,
        random,
        result.special_effect);
    if (result.special_effect) {
        result.effect_spawn = buildSpecialEffect(
            input, result.packet, random);
        result.packet = result.effect_spawn.packet;
        return result;
    }
    result.target = input.target;
    if (!input.target.found) {
        return result;
    }

    const std::size_t variant =
        static_cast<std::size_t>(input.variant);
    result.hit_chance = retailEnemyHitChance(
        input.profile->direct_hit_rate[variant],
        input.target.combat_defense);
    result.hit_roll = random.next() % 100;
    if (result.hit_roll >= result.hit_chance) {
        result.show_miss = true;
        return result;
    }

    result.apply_damage = true;
    result.damage_origin = input.source_position;
    result.post_hit_audio_sample = kHitAudioSample;
    if (input.event_number == -1) {
        result.post_hit_event = kHitEvent;
    }
    result.player_damage_can_abort_post_hit =
        input.target.kind ==
        MovementTargetKind::player;
    return result;
}

}  // namespace osf
