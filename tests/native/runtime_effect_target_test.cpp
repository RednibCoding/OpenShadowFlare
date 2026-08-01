#include "core/retail_random.hpp"
#include "world/runtime_effect_target.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

osf::RuntimeEffectTargetSnapshot target(
    osf::RuntimeEffectTargetKind kind,
    std::int32_t character_number,
    std::int32_t identifier,
    osf::WorldPosition position = {}) {
    osf::RuntimeEffectTargetSnapshot result;
    result.kind = kind;
    result.character_number = character_number;
    result.identifier = identifier;
    result.position = position;
    return result;
}

osf::RuntimeEffectTargetQuery broadQuery() {
    osf::RuntimeEffectTargetQuery query;
    query.actor_identifier = 50000017;
    query.actor_judgement = {-100, -100, 100, 100};
    query.target_mask = 0x1f;
    query.process_every_target = true;
    return query;
}

bool testFiveTargetFamiliesAndStateFilters() {
    osf::RuntimeEffectTargetQuery query = broadQuery();
    std::vector<osf::RuntimeEffectTargetSnapshot> targets{
        target(
            osf::RuntimeEffectTargetKind::player,
            0,
            100),
        target(
            osf::RuntimeEffectTargetKind::companion,
            16000000,
            200),
        target(
            osf::RuntimeEffectTargetKind::enemy,
            14000000,
            300),
        target(
            osf::RuntimeEffectTargetKind::npc,
            10000000,
            400),
        target(
            osf::RuntimeEffectTargetKind::scenario_object,
            17000000,
            500),
    };

    osf::RuntimeEffectTargetMemory memory;
    osf::RetailRandom random(3);
    const auto all =
        osf::resolveRuntimeEffectTargets(
            query, targets, memory, random);
    if (!check(
            all.contacts.size() == 5 &&
                all.contacts[0].identifier == 100 &&
                all.contacts[1].identifier == 200 &&
                all.contacts[2].identifier == 300 &&
                all.contacts[3].identifier == 400 &&
                all.contacts[4].identifier == 500,
            "The five runtime-effect mask families did not "
            "retain retail query order.")) {
        return false;
    }

    targets[0].character_number = 4;
    targets[1].local_owner = false;
    targets[2].active = false;
    targets[3].same_scenario = false;
    targets[4].displayed = false;
    const auto filtered =
        osf::resolveRuntimeEffectTargets(
            query, targets, memory, random);
    if (!check(
            filtered.contacts.empty(),
            "Dead, remote, inactive, hidden, or non-player "
            "targets survived retail state filtering.")) {
        return false;
    }

    targets[0].character_number = 0;
    targets[0].current_life = 0;
    targets[1].local_owner = true;
    targets[1].current_life = 0;
    targets[2].active = true;
    targets[2].current_life = 0;
    targets[3].same_scenario = true;
    targets[3].identifier = query.actor_identifier;
    targets[4].displayed = true;
    targets[4].runtime_state = 1;
    const auto dead =
        osf::resolveRuntimeEffectTargets(
            query, targets, memory, random);
    return check(
        dead.contacts.empty(),
        "Life, self-identity, or scenario-object runtime "
        "filters differ from the retail actor query.");
}

bool testExactTargetMaskAndOverlap() {
    osf::RuntimeEffectTargetQuery query = broadQuery();
    query.target_mask = 0x04;
    query.exact_target_only = true;
    query.target_identifier = 302;
    std::vector<osf::RuntimeEffectTargetSnapshot> targets{
        target(
            osf::RuntimeEffectTargetKind::enemy,
            14000001,
            301),
        target(
            osf::RuntimeEffectTargetKind::enemy,
            14000002,
            302),
        target(
            osf::RuntimeEffectTargetKind::enemy,
            14000003,
            302,
            {101, 0}),
        target(
            osf::RuntimeEffectTargetKind::npc,
            10000000,
            302),
    };
    osf::RuntimeEffectTargetMemory memory;
    osf::RetailRandom random(7);
    const auto result =
        osf::resolveRuntimeEffectTargets(
            query, targets, memory, random);
    return check(
        result.contacts.size() == 1 &&
            result.contacts[0].identifier == 302 &&
            result.contacts[0].character_number == 14000002,
        "Exact identity, target mask, or inclusive bounds "
        "filtering selected the wrong actor.");
}

bool testNearestTargetAndStrictTieOrder() {
    osf::RuntimeEffectTargetQuery query = broadQuery();
    query.target_mask = 0x04;
    query.process_every_target = false;
    std::vector<osf::RuntimeEffectTargetSnapshot> targets{
        target(
            osf::RuntimeEffectTargetKind::enemy,
            1,
            101,
            {20, 0}),
        target(
            osf::RuntimeEffectTargetKind::enemy,
            2,
            102,
            {5, 0}),
        target(
            osf::RuntimeEffectTargetKind::enemy,
            3,
            103,
            {-5, 0}),
    };
    osf::RuntimeEffectTargetMemory memory;
    osf::RetailRandom random(11);
    const auto result =
        osf::resolveRuntimeEffectTargets(
            query, targets, memory, random);
    return check(
        result.contacts.size() == 1 &&
            result.contacts[0].identifier == 102 &&
            result.contacts[0].distance == 5,
        "Nearest-only processing did not keep the first target "
        "on an exact distance tie.");
}

bool testEvasionReceiverAndRepeatMemory() {
    osf::RuntimeEffectTargetQuery query = broadQuery();
    query.target_mask = 0x07;
    query.remember_targets = true;
    query.has_packet = true;
    query.magical_evasion = true;
    query.hit_rating = 50;
    std::vector<osf::RuntimeEffectTargetSnapshot> targets{
        target(
            osf::RuntimeEffectTargetKind::player,
            0,
            101),
        target(
            osf::RuntimeEffectTargetKind::companion,
            16000000,
            102),
        target(
            osf::RuntimeEffectTargetKind::enemy,
            14000000,
            103),
    };
    targets[0].magical_evasion = -100;
    targets[1].magical_evasion = 30;
    targets[2].magical_evasion = 100;
    targets[0].physical_evasion = 500;
    targets[1].physical_evasion = 500;
    targets[2].physical_evasion = 500;

    osf::RetailRandom expected(1);
    const std::int32_t first_roll =
        expected.next() % 100;
    const std::int32_t second_roll =
        expected.next() % 100;
    const std::int32_t third_roll =
        expected.next() % 100;
    const std::uint32_t expected_state =
        expected.state();

    osf::RuntimeEffectTargetMemory memory;
    osf::RetailRandom random(1);
    const auto first =
        osf::resolveRuntimeEffectTargets(
            query, targets, memory, random);
    if (!check(
            first.contacts.size() == 3 &&
                first.contacts[0].hit_chance == 98 &&
                first.contacts[0].hit_roll == first_roll &&
                first.contacts[1].hit_chance == 20 &&
                first.contacts[1].hit_roll == second_roll &&
                first.contacts[2].hit_chance == 20 &&
                first.contacts[2].hit_roll == third_roll &&
                first.contacts[0].receiver_action ==
                    (first_roll < 98
                         ? osf::RuntimeEffectReceiverAction::
                               apply_packet
                         : osf::RuntimeEffectReceiverAction::
                               show_miss) &&
                first.contacts[2].receiver_action ==
                    (third_roll < 20
                         ? osf::RuntimeEffectReceiverAction::
                               apply_packet
                         : osf::RuntimeEffectReceiverAction::
                               show_miss) &&
                memory.count() == 3 &&
                random.state() == expected_state,
            "Magical evasion, clamp, random order, receiver "
            "action, or remembered-hit state changed.")) {
        return false;
    }

    const auto repeated =
        osf::resolveRuntimeEffectTargets(
            query, targets, memory, random);
    return check(
        repeated.contacts.empty() &&
            memory.count() == 3 &&
            random.state() == expected_state,
        "A remembered hit was processed again or consumed "
        "another retail random draw.");
}

bool testPacketlessRollAndExpirationPolicy() {
    osf::RuntimeEffectTargetQuery query = broadQuery();
    query.target_mask = 0x1d;
    query.expire_on_target = true;
    query.expire_on_object_contact = true;
    query.target_audio = {0, 20};
    query.object_audio = {2, 44};
    query.hit_rating = 1000;
    std::vector<osf::RuntimeEffectTargetSnapshot> targets{
        target(
            osf::RuntimeEffectTargetKind::player,
            0,
            101),
        target(
            osf::RuntimeEffectTargetKind::npc,
            10000000,
            102),
        target(
            osf::RuntimeEffectTargetKind::scenario_object,
            17000000,
            103),
    };

    osf::RetailRandom expected(19);
    const std::int32_t expected_roll =
        expected.next() % 100;
    osf::RuntimeEffectTargetMemory memory;
    osf::RetailRandom random(19);
    const auto result =
        osf::resolveRuntimeEffectTargets(
            query, targets, memory, random);
    if (!check(
            result.contacts.size() == 3 &&
                result.contacts[0].evasion_checked &&
                result.contacts[0].hit_roll ==
                    expected_roll &&
                result.contacts[0].hit_chance == 20 &&
                result.contacts[0].receiver_action ==
                    osf::RuntimeEffectReceiverAction::none &&
                result.expired &&
                result.audio.size() == 1 &&
                result.audio[0].sound.bank == 0 &&
                result.audio[0].sound.sample == 20 &&
                !result.audio[0].npc_spatial_mode &&
                random.state() == expected.state(),
            "Packetless evasion, contact expiry, or the "
            "once-per-update audio guard differs from retail.")) {
        return false;
    }

    query.target_mask = 0x08;
    query.process_every_target = true;
    targets = {
        target(
            osf::RuntimeEffectTargetKind::npc,
            10000000,
            102),
    };
    const auto every_npc =
        osf::resolveRuntimeEffectTargets(
            query, targets, memory, random);
    query.process_every_target = false;
    const auto nearest_npc =
        osf::resolveRuntimeEffectTargets(
            query, targets, memory, random);
    return check(
        every_npc.audio.size() == 1 &&
            every_npc.audio[0].npc_spatial_mode &&
            nearest_npc.audio.size() == 1 &&
            !nearest_npc.audio[0].npc_spatial_mode,
        "NPC positional audio did not preserve the distinct "
        "multi-target spatial mode.");
}

bool testMemoryCapacity() {
    osf::RuntimeEffectTargetQuery query = broadQuery();
    query.target_mask = 0x04;
    query.remember_targets = true;
    std::vector<osf::RuntimeEffectTargetSnapshot> targets;
    targets.reserve(
        osf::kRuntimeEffectRememberedTargetLimit + 1);
    for (std::size_t index = 0;
         index <
         osf::kRuntimeEffectRememberedTargetLimit + 1;
         ++index) {
        targets.push_back(target(
            osf::RuntimeEffectTargetKind::enemy,
            static_cast<std::int32_t>(index),
            1000 + static_cast<std::int32_t>(index)));
    }
    osf::RuntimeEffectTargetMemory memory;
    osf::RetailRandom random(23);
    const auto result =
        osf::resolveRuntimeEffectTargets(
            query, targets, memory, random);
    return check(
        result.contacts.size() ==
                osf::kRuntimeEffectRememberedTargetLimit + 1 &&
            memory.count() ==
                osf::kRuntimeEffectRememberedTargetLimit,
        "The retail 500-identity hit-memory cap changed "
        "contact processing or storage.");
}

}  // namespace

int main() {
    if (!testFiveTargetFamiliesAndStateFilters() ||
        !testExactTargetMaskAndOverlap() ||
        !testNearestTargetAndStrictTieOrder() ||
        !testEvasionReceiverAndRepeatMemory() ||
        !testPacketlessRollAndExpirationPolicy() ||
        !testMemoryCapacity()) {
        return 1;
    }
    return 0;
}
