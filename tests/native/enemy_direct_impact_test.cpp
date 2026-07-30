#include "core/retail_random.hpp"
#include "world/enemy_direct_impact.hpp"

#include <array>
#include <cstdint>
#include <iostream>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

osf::EnemyPresentationProfile profile() {
    osf::EnemyPresentationProfile result;
    result.packet_word_31 = 310;
    result.native_element = 320;
    result.direct_packet_word_4 = {40, 41, 42};
    result.direct_hit_rate = {200, 50, -20};
    result.direct_packet_word_40 = {400, 401, 402};
    result.direct_packet_word_41 = {410, 411, 412};
    result.direct_packet_word_43 = {430, 431, 432};
    return result;
}

osf::EnemyDirectImpactInput input(
    const osf::EnemyPresentationProfile& values) {
    osf::EnemyDirectImpactInput input;
    input.source_character_number = 14000012;
    input.source_position = {500, 600};
    input.direction_radians = 1.25;
    input.event_number = -1;
    input.variant = 0;
    input.profile = &values;
    return input;
}

bool testNormalPacketAndNoTarget() {
    const osf::EnemyPresentationProfile values =
        profile();
    osf::RetailRandom random(1);
    const osf::EnemyDirectImpactResult result =
        osf::resolveEnemyDirectImpact(
            input(values), random);
    const auto& packet = result.packet;
    return check(
        result.valid &&
            !result.special_effect &&
            !result.target.found &&
            !result.apply_damage &&
            !result.show_miss &&
            packet[0] == 2 &&
            packet[1] == 0 &&
            packet[2] == 14000012 &&
            packet[3] == 0 &&
            packet[4] == 40 &&
            packet[31] == 310 &&
            packet[32] == 320 &&
            packet[34] == 21001 &&
            packet[35] == 8 &&
            packet[36] == 200 &&
            packet[37] == 0 &&
            packet[38] == 1 &&
            packet[39] == 0 &&
            packet[40] == 400 &&
            packet[41] == 410 &&
            packet[42] == 0 &&
            packet[43] == 430 &&
            packet[44] == 0 &&
            packet[72] == 1 &&
            packet[73] == -1 &&
            packet[74] == -1 &&
            packet[75] == 8 &&
            packet[76] == 0 &&
            packet.written_words.count() == 23 &&
            packet.written_words.test(3) &&
            random.state() == 2745024u,
        "A target-less direct impact did not build the exact "
        "normal packet or consume only its visual draw.");
}

bool testHitChanceAndOutcomes() {
    if (!check(
            osf::retailEnemyHitChance(-100, 100) == 20 &&
                osf::retailEnemyHitChance(75, 25) == 50 &&
                osf::retailEnemyHitChance(1000, 0) == 98,
            "The direct-impact hit chance did not clamp to retail "
            "20 and 98 percent bounds.")) {
        return false;
    }

    osf::EnemyPresentationProfile values = profile();
    osf::EnemyDirectImpactInput hit_input =
        input(values);
    hit_input.target = {
        true,
        osf::MovementTargetKind::player,
        2,
        50,
        {100, 0},
        0,
    };
    osf::RetailRandom hit_random(1);
    const osf::EnemyDirectImpactResult hit =
        osf::resolveEnemyDirectImpact(
            hit_input, hit_random);
    osf::RetailRandom expected_hit_random(1);
    expected_hit_random.next();
    expected_hit_random.next();
    if (!check(
            hit.hit_chance == 98 &&
                hit.hit_roll == 67 &&
                hit.apply_damage &&
                hit.damage_origin.x == 500 &&
                hit.damage_origin.y == 600 &&
                !hit.show_miss &&
                hit.post_hit_audio_sample == 6 &&
                hit.post_hit_event == 17 &&
                hit.player_damage_can_abort_post_hit &&
                hit_random.state() ==
                    expected_hit_random.state(),
            "A successful player impact did not preserve the "
            "retail roll, damage request, sample, event, or "
            "post-damage abort condition.")) {
        return false;
    }

    values.direct_hit_rate[0] = 0;
    osf::EnemyDirectImpactInput miss_input =
        input(values);
    miss_input.event_number = 44;
    miss_input.target = {
        true,
        osf::MovementTargetKind::scenario_actor,
        16000001,
        40,
        {50, 0},
        500,
    };
    osf::RetailRandom miss_random(1);
    const osf::EnemyDirectImpactResult miss =
        osf::resolveEnemyDirectImpact(
            miss_input, miss_random);
    return check(
        miss.hit_chance == 20 &&
            miss.hit_roll == 67 &&
            !miss.apply_damage &&
            miss.show_miss &&
            miss.post_hit_audio_sample == -1 &&
            miss.post_hit_event == -1 &&
            !miss.player_damage_can_abort_post_hit,
        "A missed companion impact emitted successful-hit side "
        "effects or skipped the retail miss request.");
}

bool testExistingEventOnHit() {
    const osf::EnemyPresentationProfile values =
        profile();
    osf::EnemyDirectImpactInput hit_input =
        input(values);
    hit_input.event_number = 99;
    hit_input.target = {
        true,
        osf::MovementTargetKind::scenario_actor,
        16000000,
        10,
        {10, 0},
        0,
    };
    osf::RetailRandom random(1);
    const osf::EnemyDirectImpactResult result =
        osf::resolveEnemyDirectImpact(
            hit_input, random);
    return check(
        result.apply_damage &&
            result.post_hit_audio_sample == 6 &&
            result.post_hit_event == -1 &&
            !result.player_damage_can_abort_post_hit,
        "A companion hit overwrote an existing event or inherited "
        "the player-only post-damage abort.");
}

bool testSpecialEffectSwitches() {
    struct Expected {
        std::int32_t effect_number;
        std::int32_t packet_mode;
        std::int32_t visual_number;
        std::int32_t secondary_visual;
    };
    constexpr std::array<Expected, 6> expected{{
        {0, 0, 21009, -1},
        {4, 0, 21009, 20000},
        {5, 1, 21009, 21013},
        {7, 2, 21009, -1},
        {2, 0, 21003, -1},
        {21, 0, 21003, -1},
    }};
    for (const Expected& values : expected) {
        osf::EnemyPresentationProfile profile_values =
            profile();
        profile_values.direct_special_effect_number =
            values.effect_number;
        profile_values.direct_special_variant = 1;
        profile_values
            .direct_special_constructor_value_6 = 61;
        profile_values
            .direct_special_constructor_value_7 = 71;
        profile_values
            .direct_special_constructor_value_21 = 211;
        osf::EnemyDirectImpactInput special_input =
            input(profile_values);
        special_input.variant = 1;
        special_input.target = {
            true,
            osf::MovementTargetKind::player,
            0,
            1,
            {1, 0},
            0,
        };
        osf::RetailRandom random(1);
        const osf::EnemyDirectImpactResult result =
            osf::resolveEnemyDirectImpact(
                special_input, random);
        osf::RetailRandom expected_random(1);
        expected_random.next();
        expected_random.next();
        const osf::EnemyEffectSpawnRequest& spawn =
            result.effect_spawn;
        if (!check(
                result.valid &&
                    result.special_effect &&
                    !result.apply_damage &&
                    !result.show_miss &&
                    spawn.valid &&
                    spawn.effect_number ==
                        values.effect_number &&
                    spawn.owner_kind == 4 &&
                    spawn.source_character_number ==
                        14000012 &&
                    spawn.target_kind == 19 &&
                    spawn.target_identifier == -1 &&
                    spawn.constructor_value_6 == 61 &&
                    spawn.constructor_value_7 == 71 &&
                    spawn.direction_radians == 1.25 &&
                    !spawn.has_explicit_origin &&
                    !spawn.has_source_judgement &&
                    spawn.constructor_value_12 == 0 &&
                    spawn.packet_kind == 8 &&
                    spawn.instance_identifier == -1 &&
                    spawn.constructor_value_16 == 0 &&
                    spawn.constructor_value_17 == 0 &&
                    spawn.constructor_value_18 == 0 &&
                    spawn.constructor_value_19 == 0 &&
                    spawn.constructor_value_20 == 0 &&
                    spawn.constructor_value_21 == 211 &&
                    spawn.constructor_value_22 == 0 &&
                    spawn.has_packet &&
                    spawn.packet[3] ==
                        values.packet_mode &&
                    spawn.packet[34] ==
                        values.visual_number &&
                    spawn.packet[38] == 0 &&
                    spawn.packet[74] ==
                        values.secondary_visual &&
                    random.state() ==
                        expected_random.state(),
                "A direct-impact special effect did not preserve "
                "its two draws, packet switch, or 22 constructor "
                "arguments.")) {
            std::cerr
                << "special effect number: "
                << values.effect_number << '\n';
            return false;
        }
    }
    return true;
}

bool testSpecialVariantAndInvalidInput() {
    osf::EnemyPresentationProfile values = profile();
    values.direct_special_effect_number = 5;
    values.direct_special_variant = 2;
    if (!check(
            !osf::enemyDirectImpactUsesSpecialEffect(values, 1) &&
                osf::enemyDirectImpactUsesSpecialEffect(values, 2),
            "The special direct effect ignored its exact variant "
            "selector.")) {
        return false;
    }

    osf::RetailRandom random(1);
    osf::EnemyDirectImpactInput invalid = input(values);
    invalid.variant = 3;
    const osf::EnemyDirectImpactResult result =
        osf::resolveEnemyDirectImpact(invalid, random);
    return check(
        !result.valid &&
            random.state() == 1,
        "An invalid direct-impact variant partially built a packet "
        "or consumed random state.");
}

}  // namespace

int main() {
    return testNormalPacketAndNoTarget() &&
                   testHitChanceAndOutcomes() &&
                   testExistingEventOnHit() &&
                   testSpecialEffectSwitches() &&
                   testSpecialVariantAndInvalidInput()
        ? 0
        : 1;
}
