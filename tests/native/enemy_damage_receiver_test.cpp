#include "core/retail_random.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "world/enemy_damage_receiver.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

osf::TableDatabase retailTables() {
    osf::TableDatabase tables;
    std::string error;
    if (!tables.load(
            std::string(OPENSHADOWFLARE_SOURCE_DIR) +
                "/tmp/ShadowFlare/System/Game/Parameter/Table.Tbd",
            &error)) {
        std::cerr
            << "The retail Table.Tbd fixture could not be decoded: "
            << error << '\n';
    }
    return tables;
}

osf::EnemyDamageReceiverState state() {
    osf::EnemyDamageReceiverState state;
    state.character_number = 14000012;
    state.scenario_number = 6;
    state.position = {100, 0};
    state.judgement = {-20, -30, 19, 29};
    state.current_life = 500;
    state.maximum_life = 500;
    state.native_element = 0;
    state.physical_defense = 10;
    state.magical_defense = 20;
    state.presentation_action = 7;
    state.direction = 1;
    return state;
}

osf::CombatPacket packet() {
    osf::CombatPacket packet;
    packet.write(0, 0);
    packet.write(1, 1);
    packet.write(2, 0);
    packet.write(3, 0);
    packet.write(4, 100);
    packet.write(8, 0);
    packet.write(32, 0);
    packet.write(34, -1);
    packet.write(35, 8);
    packet.write(37, 0);
    packet.write(39, 0);
    packet.write(40, 0);
    packet.write(41, -1);
    packet.write(42, 0);
    packet.write(43, -1);
    packet.write(44, 0);
    packet.write(72, 0);
    packet.write(73, -1);
    packet.write(74, -1);
    packet.write(75, 8);
    packet.write(76, 0);
    return packet;
}

bool testIgnoredPackets() {
    const osf::TableDatabase tables = retailTables();
    osf::EnemyDamageReceiverState dead = state();
    dead.current_life = 0;
    osf::RetailRandom random(1);
    const osf::EnemyDamageReceiverResult dead_result =
        osf::resolveEnemyDamage(
            dead,
            packet(),
            {},
            {},
            tables,
            random);
    if (!check(
            dead_result.valid &&
                !dead_result.accepted &&
                dead_result.state.current_life == 0 &&
                dead_result.state.event_number == 0 &&
                random.state() == 1,
            "A packet delivered to an already defeated enemy "
            "changed state or consumed random state.")) {
        return false;
    }

    osf::CombatPacket invalid_source = packet();
    invalid_source.write(2, -1);
    const osf::EnemyDamageReceiverResult source_result =
        osf::resolveEnemyDamage(
            state(),
            invalid_source,
            {},
            {},
            tables,
            random);
    return check(
        source_result.valid &&
            !source_result.accepted &&
            source_result.state.current_life == 500 &&
            source_result.state.event_number == 0 &&
            random.state() == 1,
        "A negative packet source entered the enemy receiver.");
}

bool testLocalDamageAndNetworkOwnership() {
    const osf::TableDatabase tables = retailTables();
    osf::RetailRandom random(1);
    const osf::EnemyDamageReceiverResult result =
        osf::resolveEnemyDamage(
            state(),
            packet(),
            {},
            {},
            tables,
            random);
    if (!check(
            result.valid &&
                result.accepted &&
                result.damage.valid &&
                result.damage.damage == 110 &&
                result.source_player_lookup_count == 2 &&
                result.state.current_life == 390 &&
                result.state.attributed_damage[0] == 110 &&
                result.state.event_number == 4 &&
                !result.kill_requested &&
                result.network.action ==
                    osf::EnemyDamageNetworkAction::
                        broadcast_to_clients &&
                result.network.scenario_number == 6 &&
                result.network.source_slot == 0 &&
                result.network.target_character_number ==
                    14000012 &&
                !result.network.effect_family &&
                result.network.damage == 110 &&
                !result.network.source_is_character_number &&
                result.network.target_position.x == 100 &&
                random.state() == 2745024u,
            "A local ordinary hit did not preserve formula, "
            "attribution, life, event, lookup, or broadcast data.")) {
        return false;
    }

    osf::EnemyDamageReceiverContext remote_context;
    remote_context.local_player_slot = 1;
    osf::RetailRandom remote_random(1);
    const osf::EnemyDamageReceiverResult remote =
        osf::resolveEnemyDamage(
            state(),
            packet(),
            {},
            remote_context,
            tables,
            remote_random);
    if (!check(
            remote.valid &&
                remote.damage.damage == 110 &&
                remote.state.current_life == 500 &&
                remote.state.attributed_damage[0] == 0 &&
                remote.network.action ==
                    osf::EnemyDamageNetworkAction::none &&
                remote.state.event_number == 4,
            "A non-local source incorrectly changed enemy life "
            "or damage attribution.")) {
        return false;
    }

    osf::EnemyDamageReceiverState client_state = state();
    client_state.current_life = 50;
    osf::EnemyDamageReceiverContext client_context;
    client_context.network_mode = 2;
    osf::RetailRandom client_random(1);
    const osf::EnemyDamageReceiverResult client =
        osf::resolveEnemyDamage(
            client_state,
            packet(),
            {},
            client_context,
            tables,
            client_random);
    return check(
        client.valid &&
            client.state.current_life == 1 &&
            client.state.attributed_damage[0] == 0 &&
            !client.kill_requested &&
            client.network.action ==
                osf::EnemyDamageNetworkAction::
                    send_to_server &&
            client.network.damage == 110,
        "Client prediction did not send the retail damage packet "
        "or clamp its local enemy life to one.");
}

bool testZeroBaseDamageStillCompletesReceiver() {
    const osf::TableDatabase tables = retailTables();
    osf::CombatPacket zero = packet();
    zero.write(4, 0);
    osf::RetailRandom random(1);
    const osf::EnemyDamageReceiverResult result =
        osf::resolveEnemyDamage(
            state(),
            zero,
            {},
            {},
            tables,
            random);
    return check(
        result.valid &&
            result.accepted &&
            result.damage.valid &&
            result.damage.damage == 0 &&
            result.source_player_lookup_count == 1 &&
            result.state.current_life == 500 &&
            result.state.attributed_damage[0] == 0 &&
            result.network.action ==
                osf::EnemyDamageNetworkAction::none &&
            result.state.event_number == 4 &&
            random.state() == 1,
        "A non-positive base packet incorrectly ran the damage "
        "formula or skipped the receiver's final event.");
}

bool testDeathAndPlayerStatuses() {
    const osf::TableDatabase tables = retailTables();
    osf::EnemyDamageReceiverState dying = state();
    dying.current_life = 100;
    dying.presentation_counter = 9;
    dying.reaction_stage = 3;
    osf::CombatPacket killing_packet = packet();
    killing_packet.write(73, 77);
    osf::EnemyDamageReceiverContext context;
    context.apply_status_7_on_kill = true;
    context.apply_status_8_on_kill = true;
    context.apply_status_9_on_kill = true;
    osf::RetailRandom random(1);
    const osf::EnemyDamageReceiverResult result =
        osf::resolveEnemyDamage(
            dying,
            killing_packet,
            {},
            context,
            tables,
            random);
    return check(
        result.valid &&
            result.state.current_life == 0 &&
            result.state.attributed_damage[0] == 100 &&
            result.state.presentation_action == 11 &&
            result.state.presentation_counter == 0 &&
            result.state.action_lock == 1 &&
            result.state.reaction_stage == 0 &&
            result.state.death_counter == 0 &&
            !result.state.defeated_by_effect &&
            result.state.defeat_source_character_number == 0 &&
            result.kill_requested &&
            result.network.action ==
                osf::EnemyDamageNetworkAction::none &&
            result.local_player_statuses.size() == 4 &&
            result.local_player_statuses[0].status_number == 77 &&
            result.local_player_statuses[0].mode == 0 &&
            result.local_player_statuses[1].status_number == 7 &&
            result.local_player_statuses[1].mode == 1 &&
            result.local_player_statuses[2].status_number == 8 &&
            result.local_player_statuses[3].status_number == 9,
        "Enemy death lost capped attribution, killer metadata, "
        "death state, or the four ordered player status requests.");
}

bool testImpactSplatter() {
    const osf::TableDatabase tables = retailTables();
    osf::CombatPacket splatter = packet();
    splatter.write(34, 21000);

    osf::RetailRandom living_random(1);
    const osf::EnemyDamageReceiverResult living =
        osf::resolveEnemyDamage(
            state(),
            splatter,
            {},
            {},
            tables,
            living_random);
    if (!check(
            living.valid &&
                living.state.current_life > 0 &&
                living.effects.size() == 1 &&
                living.effects.front().effect_number == 21000,
            "A surviving enemy lost its ordinary impact "
            "splatter.")) {
        return false;
    }

    osf::EnemyDamageReceiverState dying = state();
    dying.current_life = 100;
    osf::RetailRandom dying_random(1);
    const osf::EnemyDamageReceiverResult death =
        osf::resolveEnemyDamage(
            dying,
            splatter,
            {},
            {},
            tables,
            dying_random);
    return check(
        death.valid &&
            death.kill_requested &&
            death.effects.size() == 1 &&
            death.effects.front().effect_number == 21000,
        "A lethal enemy hit lost its ordinary impact splatter.");
}

bool testOrdinaryHitReaction() {
    const osf::TableDatabase tables = retailTables();
    osf::EnemyDamageReceiverState reacting = state();
    reacting.has_visual = true;
    reacting.current_life = 1000;
    reacting.maximum_life = 1000;
    osf::CombatPacket hit = packet();
    hit.write(1, 0);
    hit.write(3, 1);
    hit.write(7, 6);
    hit.write(37, 1);
    hit.write(41, -1);
    hit.write(42, 100);
    hit.write(43, -1);
    hit.write(44, 0);
    hit.write(76, 2);
    osf::RetailRandom random(1);
    const osf::EnemyDamageReceiverResult result =
        osf::resolveEnemyDamage(
            reacting,
            hit,
            {0, 0},
            {},
            tables,
            random);
    osf::RetailRandom expected_random(1);
    expected_random.next();
    expected_random.next();
    return check(
        result.valid &&
            result.damage.damage == 100 &&
            result.source_player_lookup_count == 1 &&
            result.state.current_life == 900 &&
            result.state.presentation_action == 10 &&
            result.state.presentation_counter == 0 &&
            result.state.action_lock == 1 &&
            result.state.reaction_duration == 16 &&
            result.state.reaction_stage == 1 &&
            !result.state.reaction_displacement_suppressed &&
            result.state.reaction_additive == 2 &&
            std::abs(result.state.reaction_angle) < 0.000001 &&
            result.state.direction == 5 &&
            result.effects.size() == 1 &&
            result.effects[0].effect_number == 21018 &&
            result.effects[0].constructor_value_12 == 16 &&
            !result.effects[0].has_packet &&
            result.effects[0].has_source_judgement &&
            result.effects[0].packet_kind == 8 &&
            result.audio_samples.size() == 1 &&
            result.audio_samples[0] == 119 &&
            random.state() == expected_random.state(),
        "An ordinary hit did not preserve affinity tables, "
        "reaction timing, facing, effect, audio, or draw order.");
}

bool testEffectReactionOverride() {
    const osf::TableDatabase tables = retailTables();
    osf::EnemyDamageReceiverState reacting = state();
    reacting.has_visual = true;
    reacting.current_life = 1000;
    reacting.maximum_life = 1000;
    reacting.native_element = 2;
    reacting.always_suppress_reaction_displacement = true;
    osf::CombatPacket effect = packet();
    effect.write(0, 2);
    effect.write(1, 3);
    effect.write(3, 2);
    effect.write(4, 200);
    effect.write(32, 3);
    effect.write(37, 1);
    effect.write(40, 1);
    effect.write(41, 0);
    effect.write(43, 0);
    effect.write(47, 0);
    effect.write(56, 100);
    effect.write(65, 30);
    osf::RetailRandom random(1);
    const osf::EnemyDamageReceiverResult result =
        osf::resolveEnemyDamage(
            reacting,
            effect,
            {},
            {},
            tables,
            random);
    return check(
            result.valid &&
                result.damage.damage == 200 &&
                result.source_player_lookup_count == 0 &&
                result.source_scenario_actor_lookup_requested &&
                result.source_owner_lookup_requested &&
            result.state.current_life == 800 &&
            result.state.presentation_action == 10 &&
            result.state.reaction_duration == 15 &&
            result.state.reaction_stage == 2 &&
            result.state.reaction_displacement_suppressed &&
            result.effects.empty() &&
            result.audio_samples.empty() &&
            result.network.effect_family &&
            random.state() == 2745024u,
        "An effect hit did not use its native-element banks, "
        "pre-force duration cap, stage two, or effect-family flag.");
}

bool testReflectionAndPacketEffects() {
    const osf::TableDatabase tables = retailTables();
    osf::EnemyDamageReceiverState receiving = state();
    receiving.has_visual = true;
    receiving.current_life = 1000;
    receiving.maximum_life = 1000;
    osf::CombatPacket effects = packet();
    effects.write(4, 1);
    effects.write(37, 1);
    effects.write(39, 1);
    effects.write(41, 0);
    effects.write(43, 0);
    effects.write(34, 123);
    effects.write(35, 7);
    effects.write(74, 456);
    effects.write(75, 9);
    effects.write(72, 1);
    osf::EnemyDamageReceiverContext context;
    context.source_player_available = true;
    context.source_player_position = {0, 0};
    osf::RetailRandom random(3);
    const osf::EnemyDamageReceiverResult result =
        osf::resolveEnemyDamage(
            receiving,
            effects,
            {},
            context,
            tables,
            random);
    osf::RetailRandom expected_random(3);
    for (int draw = 0; draw < 9; ++draw) {
        expected_random.next();
    }
    if (!check(
            result.valid &&
                result.effects.size() == 8 &&
                result.audio_samples.size() == 1 &&
                result.audio_samples[0] == 61 &&
                random.state() == expected_random.state(),
            "Reflection, configured, and random hit effects did "
            "not preserve their count, audio, or nine draws.")) {
        return false;
    }
    for (std::size_t index = 0; index < 5; ++index) {
        const auto& reflection = result.effects[index];
        if (!check(
                reflection.effect_number == 20013 &&
                    reflection.target_kind == 1 &&
                    reflection.target_identifier == 0 &&
                    reflection.constructor_value_6 == 100 &&
                    reflection.constructor_value_7 == 250 &&
                    !reflection.has_source_judgement &&
                    !reflection.has_packet,
                "A reflected effect lost its retail constructor "
                "arguments.")) {
            return false;
        }
    }
    if (!check(
            std::abs(
                result.effects[0].direction_radians -
                (-0.304000653589793)) <
                0.000000001,
            "The first reflected effect changed its retail "
            "spread draw or angle constants.")) {
        return false;
    }
    return check(
        result.effects[5].effect_number == 123 &&
            result.effects[5].packet_kind == 7 &&
            result.effects[5].has_source_judgement &&
            !result.effects[5].has_packet &&
            result.effects[6].effect_number == 456 &&
            result.effects[6].packet_kind == 9 &&
            result.effects[7].effect_number == 21012 &&
            result.effects[7].packet_kind == 7,
        "Configured or random hit effects changed their retail "
        "order, effect number, packet kind, or null-packet state.");
}

bool testMissingReactionTableIsContained() {
    osf::EnemyDamageReceiverState receiving = state();
    receiving.has_visual = true;
    receiving.current_life = 1000;
    receiving.maximum_life = 1000;
    osf::CombatPacket hit = packet();
    hit.write(37, 1);
    hit.write(41, -1);
    hit.write(43, -1);
    osf::TableDatabase no_tables;
    osf::RetailRandom random(1);
    const osf::EnemyDamageReceiverResult result =
        osf::resolveEnemyDamage(
            receiving,
            hit,
            {},
            {},
            no_tables,
            random);
    return check(
        !result.valid &&
            result.accepted &&
            result.state.current_life == 1000 &&
            result.state.presentation_action == 7 &&
            result.state.event_number == 0 &&
            random.state() == 1,
        "A missing reaction table returned partial enemy state "
        "or consumed the later chance draw.");
}

}  // namespace

int main() {
    return testIgnoredPackets() &&
                   testLocalDamageAndNetworkOwnership() &&
                   testZeroBaseDamageStillCompletesReceiver() &&
                   testDeathAndPlayerStatuses() &&
                   testImpactSplatter() &&
                   testOrdinaryHitReaction() &&
                   testEffectReactionOverride() &&
                   testReflectionAndPacketEffects() &&
                   testMissingReactionTableIsContained()
        ? 0
        : 1;
}
