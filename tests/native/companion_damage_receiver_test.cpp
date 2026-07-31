#include "core/retail_random.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "world/companion_damage_receiver.hpp"

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

osf::CompanionDamageReceiverState state() {
    osf::CompanionDamageReceiverState state;
    state.character_number = 16000003;
    state.position = {100, 50};
    state.judgement = {-20, -20, 19, 19};
    state.current_life = 100;
    state.maximum_life = 100;
    state.native_element = 0;
    state.physical_defense = 20;
    state.magical_defense = 10;
    state.presentation_action = 2;
    state.direction = 1;
    return state;
}

osf::CombatPacket directPacket(
    std::int32_t damage) {
    osf::CombatPacket packet;
    packet.write(0, 2);
    packet.write(1, 0);
    packet.write(2, 14000000);
    packet.write(3, 0);
    packet.write(4, damage);
    packet.write(32, 0);
    packet.write(34, -1);
    packet.write(35, 8);
    packet.write(37, 1);
    packet.write(40, 0);
    packet.write(41, 0);
    packet.write(42, 0);
    packet.write(43, 1);
    packet.write(44, 0);
    packet.write(72, 0);
    packet.write(74, -1);
    packet.write(75, 8);
    packet.write(76, 0);
    return packet;
}

bool testIgnoredStates() {
    const osf::TableDatabase tables = retailTables();
    for (const std::int32_t excluded_action :
         {7, 8, 10}) {
        osf::CompanionDamageReceiverState ignored = state();
        ignored.presentation_action = excluded_action;
        osf::RetailRandom random(1);
        const osf::CompanionDamageReceiverResult action =
            osf::resolveCompanionDamage(
                ignored,
                directPacket(20),
                {},
                {},
                tables,
                random);
        if (!check(
                action.valid &&
                    !action.accepted &&
                    action.state.current_life == 100 &&
                    action.state.event_number == 0 &&
                    random.state() == 1,
                "A retail-excluded companion action accepted "
                "or consumed a hit.")) {
            return false;
        }
    }

    osf::CompanionDamageReceiverState ignored = state();
    ignored.current_life = 0;
    osf::RetailRandom dead_random(1);
    const osf::CompanionDamageReceiverResult dead =
        osf::resolveCompanionDamage(
            ignored,
            directPacket(20),
            {},
            {},
            tables,
            dead_random);
    return check(
        dead.valid &&
            !dead.accepted &&
            dead_random.state() == 1,
        "An already defeated companion accepted a hit.");
}

bool testLocalOwnershipAndDeath() {
    const osf::TableDatabase tables = retailTables();
    osf::CompanionDamageReceiverContext local;
    local.local_player_slot = 3;
    osf::CombatPacket living_packet =
        directPacket(25);
    living_packet.write(34, 21000);
    osf::RetailRandom local_random(2);
    const osf::CompanionDamageReceiverResult local_result =
        osf::resolveCompanionDamage(
            state(),
            living_packet,
            {},
            local,
            tables,
            local_random);
    if (!check(
            local_result.valid &&
                local_result.accepted &&
                local_result.state.current_life == 75 &&
                local_result.effects.empty(),
            "The owning player slot did not apply companion damage "
            "or emitted a splatter while the companion survived.")) {
        return false;
    }

    osf::CombatPacket negative_source = directPacket(5);
    negative_source.write(2, -1);
    osf::RetailRandom source_random(2);
    const osf::CompanionDamageReceiverResult source_result =
        osf::resolveCompanionDamage(
            state(),
            negative_source,
            {},
            local,
            tables,
            source_random);
    if (!check(
            source_result.valid &&
                source_result.accepted &&
                source_result.state.current_life == 95,
            "The companion receiver incorrectly copied the "
            "enemy negative-source guard.")) {
        return false;
    }

    osf::CompanionDamageReceiverContext remote;
    remote.local_player_slot = 1;
    osf::RetailRandom remote_random(2);
    const osf::CompanionDamageReceiverResult remote_result =
        osf::resolveCompanionDamage(
            state(),
            directPacket(25),
            {},
            remote,
            tables,
            remote_random);
    if (!check(
            remote_result.valid &&
                remote_result.state.current_life == 100,
            "A non-owning player slot changed companion life.")) {
        return false;
    }

    osf::CompanionDamageReceiverState dying = state();
    dying.current_life = 5;
    osf::CombatPacket lethal_packet =
        directPacket(10);
    lethal_packet.write(34, 21000);
    osf::RetailRandom death_random(3);
    const osf::CompanionDamageReceiverResult death =
        osf::resolveCompanionDamage(
            dying,
            lethal_packet,
            {},
            local,
            tables,
            death_random);
    return check(
        death.valid &&
            death.state.current_life == -5 &&
            death.state.presentation_action == 6 &&
            death.state.presentation_counter == 0 &&
            death.state.action_lock == 1 &&
            death.state.event_number == 4 &&
            death.effects.size() == 1 &&
            death.effects.front().effect_number == 21000,
        "Lethal companion damage did not select action six and "
        "emit its death splatter.");
}

bool testReactionStageAndEffectOwnership() {
    const osf::TableDatabase tables = retailTables();
    osf::CombatPacket packet = directPacket(10);
    packet.write(3, 1);
    packet.write(41, 100);
    packet.write(43, 30);
    packet.write(34, 20123);
    packet.write(35, 6);
    packet.write(74, 20124);
    packet.write(75, 7);
    osf::CompanionDamageReceiverContext context;
    context.local_player_slot = 3;
    osf::RetailRandom random(4);
    const osf::CompanionDamageReceiverResult result =
        osf::resolveCompanionDamage(
            state(),
            packet,
            {0, 50},
            context,
            tables,
            random);
    return check(
        result.valid &&
            result.state.presentation_action == 5 &&
            result.state.reaction_stage == 1 &&
            result.state.reaction_duration == 15 &&
            !result.state.reaction_motion &&
            result.effects.size() == 3 &&
            result.effects[0].effect_number >= 21015 &&
            result.effects[0].effect_number <= 21018 &&
            result.effects[0].owner_kind == 4 &&
            result.effects[0].constructor_value_12 == 15 &&
            result.effects[1].effect_number == 20123 &&
            result.effects[1].owner_kind == 2 &&
            result.effects[1].packet_kind == 6 &&
            result.effects[2].effect_number == 20124 &&
            result.effects[2].owner_kind == 2 &&
            result.effects[2].packet_kind == 7 &&
            result.audio_samples.size() == 1 &&
            result.audio_samples[0] == 119,
        "Companion reaction stages or effect owner kinds differed.");
}

bool testEffectFamilyBanksAndRandomHit() {
    const osf::TableDatabase tables = retailTables();
    osf::CombatPacket packet = directPacket(10);
    packet.write(1, 3);
    packet.write(40, 1);
    packet.write(41, 0);
    packet.write(43, 1);
    packet.write(45, 0);
    packet.write(54, 100);
    packet.write(63, 22);
    packet.write(72, 1);

    std::uint32_t seed = 1;
    for (;; ++seed) {
        osf::RetailRandom probe(seed);
        probe.next();
        if (probe.next() % 100 < 20) {
            break;
        }
    }
    osf::CompanionDamageReceiverContext context;
    context.local_player_slot = 3;
    osf::RetailRandom random(seed);
    const osf::CompanionDamageReceiverResult result =
        osf::resolveCompanionDamage(
            state(),
            packet,
            {},
            context,
            tables,
            random);
    return check(
        result.valid &&
            result.state.presentation_action == 5 &&
            result.state.reaction_duration == 15 &&
            !result.state.reaction_motion &&
            result.effects.size() == 1 &&
            result.effects[0].effect_number >= 21011 &&
            result.effects[0].effect_number <= 21012 &&
            result.effects[0].owner_kind == 2 &&
            result.effects[0].packet_kind >= 0 &&
            result.effects[0].packet_kind < 8,
        "Effect-family reaction banks or the random hit effect "
        "differed.");
}

}  // namespace

int main() {
    if (!testIgnoredStates() ||
        !testLocalOwnershipAndDeath() ||
        !testReactionStageAndEffectOwnership() ||
        !testEffectFamilyBanksAndRandomHit()) {
        return 1;
    }
    return 0;
}
