#include "core/retail_random.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "world/actor_direction.hpp"
#include "world/enemy_effect_controller.hpp"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

osf::CombatEffectSpawnRequest requestFor(
    std::int32_t effect_number,
    std::int32_t delay) {
    osf::CombatEffectSpawnRequest request;
    request.valid = true;
    request.effect_number = effect_number;
    request.owner_kind = 4;
    request.source_character_number = 14000042;
    request.target_kind = 1;
    request.target_identifier = 3;
    request.constructor_value_6 = 73;
    request.constructor_value_7 = 250;
    request.direction_radians = 0.0;
    request.has_source_judgement = true;
    request.source_judgement = {-20, -30, 21, 31};
    request.constructor_value_12 = delay;
    request.has_packet = true;
    request.packet.write(2, 14000042);
    request.packet.write(34, 21013);
    return request;
}

osf::EnemyEffectControllerUpdate updateController(
    osf::EnemyEffectController& controller,
    osf::EnemyEffectControllerSource source) {
    return controller.update(
        {source, nullptr, {}, {}, {}});
}

bool testTypeOneZeroDelay() {
    osf::EnemyEffectController controller;
    const osf::CombatEffectSpawnRequest request =
        requestFor(10001, 0);
    if (!check(
            controller.initialize(request) &&
                controller.active() &&
                controller.counter() == 0 &&
                controller.effectNumber() == 10001,
            "A valid type-one request did not create a fresh "
            "controller.")) {
        return false;
    }

    const osf::EnemyEffectControllerUpdate update =
        updateController(
            controller, {true, {100, 200}});
    if (!check(
            update.expired &&
                update.actor_spawn_count == 2 &&
                update.audio_count == 1 &&
                !controller.active() &&
                controller.effectNumber() == -1,
            "A zero-delay type-one controller did not emit both "
            "actors and expire on its first update.")) {
        return false;
    }

    const auto& source = update.actor_spawns[0];
    if (!check(
            source.controller_effect_number == 10001 &&
                source.resource_id == 10000012 &&
                source.owner_kind == 4 &&
                source.source_character_number == 14000042 &&
                source.position.x == 100 &&
                source.position.y == 200 &&
                source.judgement.left == 22 &&
                source.judgement.top == 32 &&
                source.judgement.right == 22 &&
                source.judgement.bottom == 32 &&
                source.lifetime == -1 &&
                source.lifetime_from_animation &&
                source.collide_with_environment &&
                !source.expire_on_environment_collision &&
                source.target_collision_start == -1 &&
                source.target_collision_end == -1 &&
                source.animation_chart == 0 &&
                source.animation_direction == 8 &&
                !source.has_packet,
            "The type-one source actor differs from the retail "
            "point animation descriptor.")) {
        return false;
    }

    const auto& child = update.actor_spawns[1];
    if (!check(
            child.controller_effect_number == 10001 &&
                child.resource_id == 10000010 &&
                child.owner_kind == 4 &&
                child.source_character_number == 14000042 &&
                child.target_mask == 1 &&
                child.target_identifier == 3 &&
                child.travel_speed == 73 &&
                child.display_height == 250 &&
                child.position.x == 280 &&
                child.position.y == 200 &&
                child.judgement.left == -50 &&
                child.judgement.top == -50 &&
                child.judgement.right == 50 &&
                child.judgement.bottom == 50 &&
                child.lifetime == -1 &&
                !child.lifetime_from_animation &&
                child.collide_with_environment &&
                child.expire_on_environment_collision &&
                child.target_collision_start == 0 &&
                child.target_collision_end == -1 &&
                child.expire_on_target &&
                !child.remember_targets &&
                child.target_audio.bank == 0 &&
                child.target_audio.sample == 20 &&
                child.animation_chart == 0 &&
                child.animation_direction == 1 &&
                child.has_packet &&
                child.packet[2] == 14000042 &&
                child.packet[34] == 21013 &&
                child.packet.written_words ==
                    request.packet.written_words,
            "The type-one child actor did not preserve its "
            "directional descriptor and combat packet.")) {
        return false;
    }

    return check(
        update.audio[0].sample == 19 &&
            update.audio[0].position.x == 280 &&
            update.audio[0].position.y == 200,
        "Type one did not place sample 19 at the child actor.");
}

bool testTypeTwoDelayedReresolution() {
    osf::CombatEffectSpawnRequest request =
        requestFor(10002, 2);
    request.constructor_value_22 = 1;
    request.direction_radians =
        3.14159265358979323846 / 2.0;

    osf::EnemyEffectController controller;
    if (!check(
            controller.initialize(request),
            "A valid type-two request was rejected.")) {
        return false;
    }

    const auto first =
        updateController(
            controller, {true, {100, 200}});
    if (!check(
            first.actor_spawn_count == 1 &&
                first.actor_spawns[0].resource_id ==
                    11000027 &&
                first.actor_spawns[0].position.x == 100 &&
                first.actor_spawns[0].position.y == 200 &&
                first.audio_count == 0 &&
                !first.expired &&
                controller.counter() == 1,
            "Type two did not create only its source animation "
            "on update zero.")) {
        return false;
    }

    const auto second =
        updateController(
            controller, {true, {200, 300}});
    if (!check(
            second.actor_spawn_count == 0 &&
                second.audio_count == 0 &&
                !second.expired &&
                controller.counter() == 2,
            "Type two emitted its delayed actor one update "
            "early.")) {
        return false;
    }

    const auto third =
        updateController(
            controller, {true, {300, 400}});
    return check(
        third.actor_spawn_count == 1 &&
            third.actor_spawns[0].resource_id ==
                10000040 &&
            third.actor_spawns[0].position.x == 300 &&
            third.actor_spawns[0].position.y == 220 &&
            third.actor_spawns[0].animation_direction == 3 &&
            third.actor_spawns[0].remember_targets &&
            third.audio_count == 1 &&
            third.audio[0].sample == 94 &&
            third.audio[0].position.x == 300 &&
            third.audio[0].position.y == 220 &&
            third.expired &&
            !controller.active(),
        "Type two did not re-resolve its source before placing "
        "the delayed child and sample 94.");
}

bool testFixedOriginAndMissingSource() {
    osf::CombatEffectSpawnRequest request =
        requestFor(10001, 0);
    request.owner_kind = 0;
    request.has_explicit_origin = true;
    request.origin = {700, 900};

    osf::EnemyEffectController controller;
    controller.initialize(request);
    const auto fixed =
        updateController(controller, {true, {1, 2}});
    if (!check(
            fixed.actor_spawns[0].position.x == 700 &&
                fixed.actor_spawns[0].position.y == 900 &&
                fixed.actor_spawns[1].position.x == 700 &&
                fixed.actor_spawns[1].position.y == 900,
            "An owner-kind-zero controller projected its child "
            "away from the stored origin.")) {
        return false;
    }

    request.owner_kind = 4;
    controller.initialize(request);
    const auto missing =
        updateController(
            controller, {false, {600, 800}});
    if (!check(
        missing.actor_spawns[0].position.x == 0 &&
            missing.actor_spawns[0].position.y == 0 &&
            missing.actor_spawns[1].position.x == 180 &&
            missing.actor_spawns[1].position.y == 0,
        "A missing retail source retained a stale supplied "
        "position instead of resolving from zero.")) {
        return false;
    }

    request.owner_kind = 0;
    request.has_explicit_origin = false;
    request.origin = {500, 600};
    request.has_source_judgement = false;
    request.source_judgement = {10, 20, 30, 40};
    controller.initialize(request);
    const auto omitted =
        updateController(
            controller, {true, {700, 800}});
    return check(
        omitted.actor_spawns[0].position.x == 0 &&
            omitted.actor_spawns[0].position.y == 0 &&
            omitted.actor_spawns[0].judgement.left == 1 &&
            omitted.actor_spawns[0].judgement.top == 1 &&
            omitted.actor_spawns[0].judgement.right == 1 &&
            omitted.actor_spawns[0].judgement.bottom == 1,
        "Absent optional constructor pointers leaked stale "
        "origin or judgement values into an actor.");
}

bool testNegativeDelayAndRejectedRequests() {
    osf::EnemyEffectController controller;
    osf::CombatEffectSpawnRequest request =
        requestFor(10002, -1);
    if (!check(
            controller.initialize(request),
            "A supported controller with a negative authored "
            "delay was rejected.")) {
        return false;
    }

    for (std::int32_t update_number = 0;
         update_number < 4;
         ++update_number) {
        const auto update =
            updateController(
                controller, {true, {10, 20}});
        if (!check(
                update.actor_spawn_count ==
                    (update_number == 0 ? 1u : 0u) &&
                    update.audio_count == 0 &&
                    !update.expired,
                "A negative delay incorrectly reached its "
                "child-spawn update.")) {
            return false;
        }
    }
    if (!check(
            controller.active() &&
                controller.counter() == 4,
            "A negative-delay controller did not remain in the "
            "retail list.")) {
        return false;
    }

    request.valid = false;
    if (!check(
            !controller.initialize(request) &&
                !controller.active(),
            "An invalid effect request created a controller.")) {
        return false;
    }
    request.valid = true;
    request.effect_number = 10021;
    return check(
        !controller.initialize(request) &&
            updateController(controller, {}).expired,
        "An unimplemented specialized family entered the "
        "implemented controller owner.");
}

bool testTypeFourWarningBurstAndCameraShake() {
    osf::CombatEffectSpawnRequest request =
        requestFor(10004, 10);
    osf::EnemyEffectController controller;
    if (!check(
            controller.initialize(request),
            "A valid type-four request was rejected.")) {
        return false;
    }

    for (std::int32_t update_number = 0;
         update_number < 10;
         ++update_number) {
        const osf::WorldPosition source{
            100 + update_number * 40,
            200 + update_number * 40,
        };
        const auto update = controller.update({
            {true, source},
            nullptr,
            {},
            {true, {500, 3500}},
            {},
        });
        if (update_number == 3) {
            const auto& warning = update.actor_spawns[0];
            if (!check(
                    update.actor_spawn_count == 1 &&
                        update.audio_count == 0 &&
                        !update.camera_shake &&
                        !update.expired &&
                        warning.resource_id == 10000002 &&
                        warning.position.x == 220 &&
                        warning.position.y == 320 &&
                        warning.judgement.left == -20 &&
                        warning.judgement.top == -30 &&
                        warning.judgement.right == 21 &&
                        warning.judgement.bottom == 31 &&
                        warning.display_height == 0 &&
                        warning.lifetime_from_animation &&
                        warning.lifetime_animation_chart == 0 &&
                        warning.animation_chart == 0 &&
                        warning.animation_direction == 8 &&
                        warning.additional_display_status ==
                            0x80 &&
                        warning.visible &&
                        !warning.has_packet,
                    "Type four did not create its retail "
                    "warning actor on update three.")) {
                return false;
            }
        } else if (!check(
                       update.actor_spawn_count == 0 &&
                           update.audio_count == 0 &&
                           !update.camera_shake &&
                           !update.expired,
                       "Type four emitted work before its "
                       "warning or authored burst update.")) {
            return false;
        }
    }

    const auto burst = controller.update({
        {true, {500, 600}},
        nullptr,
        {},
        {true, {500, 3500}},
        {},
    });
    if (!check(
            burst.actor_spawn_count == 3 &&
                burst.audio_count == 2 &&
                burst.camera_shake &&
                burst.camera_shake_duration == 8 &&
                burst.camera_shake_magnitude == 6 &&
                burst.expired &&
                !controller.active(),
            "Type four did not emit and expire its complete "
            "delayed burst.")) {
        return false;
    }

    const auto& first = burst.actor_spawns[0];
    const auto& second = burst.actor_spawns[1];
    const auto& damage = burst.actor_spawns[2];
    if (!check(
            first.resource_id == 10000000 &&
                first.position.x == 500 &&
                first.position.y == 600 &&
                first.judgement.left == -21 &&
                first.judgement.top == -31 &&
                first.judgement.right == -21 &&
                first.judgement.bottom == -31 &&
                first.display_height == 200 &&
                first.lifetime_from_animation &&
                first.lifetime_animation_chart == 1 &&
                first.animation_chart == 1 &&
                first.animation_direction == 8 &&
                second.resource_id == 10000000 &&
                second.position.x == 500 &&
                second.position.y == 600 &&
                second.judgement.left == 22 &&
                second.judgement.top == 32 &&
                second.judgement.right == 22 &&
                second.judgement.bottom == 32 &&
                second.display_height == 200 &&
                second.lifetime_from_animation &&
                second.lifetime_animation_chart == 1 &&
                second.animation_chart == 0 &&
                second.animation_direction == 8,
            "Type four changed one of its two differently "
            "charted visual descriptors.")) {
        return false;
    }
    if (!check(
            damage.resource_id == -1 &&
                damage.owner_kind == 4 &&
                damage.source_character_number == 14000042 &&
                damage.target_mask == 1 &&
                damage.target_identifier == 3 &&
                damage.position.x == 500 &&
                damage.position.y == 600 &&
                damage.judgement.left == -170 &&
                damage.judgement.top == -180 &&
                damage.judgement.right == 171 &&
                damage.judgement.bottom == 181 &&
                damage.display_height == 200 &&
                damage.lifetime == 1 &&
                !damage.lifetime_from_animation &&
                damage.target_collision_start == 0 &&
                damage.target_collision_end == 0 &&
                damage.process_every_target &&
                damage.target_audio.bank == 0 &&
                damage.target_audio.sample == 20 &&
                !damage.visible &&
                damage.has_packet &&
                damage.packet[34] == 21013,
            "Type four did not preserve its invisible one-update "
            "damage actor and copied packet.")) {
        return false;
    }
    if (!check(
            burst.audio[0].sample == 29 &&
                burst.audio[1].sample == 23 &&
                burst.audio[0].position.x == 500 &&
                burst.audio[0].position.y == 600 &&
                burst.audio[1].position.x == 500 &&
                burst.audio[1].position.y == 600,
            "Type four did not place samples 29 and 23 at its "
            "re-resolved source.")) {
        return false;
    }

    controller.initialize(request);
    osf::EnemyEffectControllerUpdate far_burst;
    for (std::int32_t update_number = 0;
         update_number <= 10;
         ++update_number) {
        far_burst = controller.update({
            {true, {500, 600}},
            nullptr,
            {},
            {true, {500, 3601}},
            {},
        });
    }
    return check(
        far_burst.expired &&
            !far_burst.camera_shake,
        "Type four shook an observer outside the strict "
        "3001-unit retail range.");
}

bool testTypeFiveFrameCountSequence() {
    osf::CombatEffectSpawnRequest request =
        requestFor(10005, 10);
    osf::EnemyEffectController controller;
    if (!check(
            controller.initialize(request),
            "A valid type-five request was rejected.")) {
        return false;
    }

    constexpr std::int32_t first_length = 7;
    std::int32_t length_calls = 0;
    std::size_t actor_count = 0;
    std::size_t audio_count = 0;
    std::size_t shake_count = 0;
    std::vector<std::int32_t> audio_updates;
    for (std::int32_t update_number = 0;
         update_number <
             first_length + 22;
         ++update_number) {
        const osf::WorldPosition source{
            100 + update_number * 40,
            200 + update_number * 40,
        };
        const auto update = controller.update({
            {true, source},
            nullptr,
            {},
            {true, {220, 3320}},
            [&length_calls](
                std::int32_t resource_id,
                std::int32_t chart,
                std::int32_t direction) {
                ++length_calls;
                return resource_id == 10000051 &&
                               chart == 0 &&
                               direction == 8
                    ? first_length
                    : 0;
            },
        });
        actor_count += update.actor_spawn_count;
        audio_count += update.audio_count;
        shake_count += update.camera_shake ? 1u : 0u;
        if (update.audio_count != 0) {
            audio_updates.push_back(update_number);
            if (!check(
                    update.audio_count == 1 &&
                        update.audio[0].sample == 22 &&
                        update.audio[0].position.x == 220 &&
                        update.audio[0].position.y == 320,
                    "A type-five pulse did not keep sample 22 at "
                    "the captured source position.")) {
                return false;
            }
        }

        if (update_number == 3) {
            const auto& first = update.actor_spawns[0];
            if (!check(
                    update.actor_spawn_count == 1 &&
                        first.resource_id == 10000051 &&
                        first.position.x == 220 &&
                        first.position.y == 320 &&
                        first.judgement.left == 22 &&
                        first.judgement.top == 32 &&
                        first.judgement.right == 22 &&
                        first.judgement.bottom == 32 &&
                        first.display_height == 0 &&
                        first.lifetime_from_animation &&
                        first.lifetime_animation_chart == 0 &&
                        first.animation_chart == 0 &&
                        first.animation_direction == 8 &&
                        first.additional_display_status == 0 &&
                        !first.has_packet,
                    "Type five did not capture its source and "
                    "create resource 10000051 on update three.")) {
                return false;
            }
        } else if (update_number == first_length) {
            const auto& second = update.actor_spawns[0];
            if (!check(
                    update.actor_spawn_count == 1 &&
                        second.resource_id == 10000050 &&
                        second.position.x == 220 &&
                        second.position.y == 320 &&
                        second.judgement.left == 22 &&
                        second.judgement.top == 32 &&
                        second.judgement.right == 22 &&
                        second.judgement.bottom == 32 &&
                        second.display_height == 200 &&
                        second.lifetime_from_animation &&
                        second.lifetime_animation_chart == 0 &&
                        second.animation_chart == 0 &&
                        second.animation_direction == 8 &&
                        second.additional_display_status == 0,
                    "Type five used its authored delay instead "
                    "of the first resource's frame count.")) {
                return false;
            }
        } else if (
            update_number ==
            first_length + 4) {
            const auto& damage = update.actor_spawns[0];
            if (!check(
                    update.actor_spawn_count == 1 &&
                        update.camera_shake &&
                        update.camera_shake_duration == 8 &&
                        update.camera_shake_magnitude == 6 &&
                        damage.resource_id == -1 &&
                        damage.position.x == 220 &&
                        damage.position.y == 320 &&
                        damage.judgement.left == -170 &&
                        damage.judgement.top == -180 &&
                        damage.judgement.right == 171 &&
                        damage.judgement.bottom == 181 &&
                        damage.display_height == 200 &&
                        damage.lifetime == 1 &&
                        damage.target_collision_start == 0 &&
                        damage.target_collision_end == 0 &&
                        damage.process_every_target &&
                        damage.target_audio.bank == 0 &&
                        damage.target_audio.sample == 20 &&
                        !damage.visible &&
                        damage.has_packet,
                    "Type five did not create its fixed-position "
                    "area packet and camera shake four updates "
                    "after the first animation.")) {
                return false;
            }
        } else if (
            update_number ==
            first_length + 15) {
            const auto& third = update.actor_spawns[0];
            if (!check(
                    update.actor_spawn_count == 1 &&
                        third.resource_id == 10000052 &&
                        third.position.x == 220 &&
                        third.position.y == 320 &&
                        third.judgement.left == 22 &&
                        third.judgement.top == 32 &&
                        third.judgement.right == 22 &&
                        third.judgement.bottom == 32 &&
                        third.display_height == 200 &&
                        third.lifetime_from_animation &&
                        third.lifetime_animation_chart == 0 &&
                        third.animation_chart == 0 &&
                        third.animation_direction == 8 &&
                        third.additional_display_status ==
                            0x80,
                    "Type five did not create its display-class "
                    "two final visual at frame-count plus 15.")) {
                return false;
            }
        } else if (!check(
                       update.actor_spawn_count == 0,
                       "Type five emitted an actor outside its "
                       "four retail sequence points.")) {
            return false;
        }

        if (update_number <
                first_length + 21 &&
            !check(
                !update.expired &&
                    controller.active(),
                "Type five expired before frame-count plus "
                "22.")) {
            return false;
        }
        if (update_number ==
                first_length + 21 &&
            !check(
                update.expired &&
                    !controller.active() &&
                    controller.counter() ==
                        first_length + 22,
                "Type five did not expire immediately after its "
                "last sound-pulse update.")) {
            return false;
        }
    }

    return check(
        length_calls == first_length + 22 &&
            actor_count == 4 &&
            audio_count == 6 &&
            shake_count == 1 &&
            audio_updates ==
                std::vector<std::int32_t>{
                    13, 16, 19, 22, 25, 28},
        "Type five did not follow the resource-length timeline "
        "or six three-update sample-22 pulses.");
}

bool testTypeThreeWaves() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    osf::TableDatabase tables;
    std::string error;
    if (!check(
            tables.load(
                std::filesystem::path(
                    OPENSHADOWFLARE_SOURCE_DIR) /
                    "tmp" / "ShadowFlare" / "System" /
                    "Game" / "Parameter" / "Table.Tbd",
                &error),
            "The retail type-three wave table could not be "
            "loaded.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::CombatEffectSpawnRequest request =
        requestFor(10003, 2);
    request.owner_kind = 4;
    request.has_explicit_origin = true;
    request.origin = {1000, 2000};
    request.constructor_value_17 = 20;
    request.target_kind = 19;
    request.target_identifier = -1;

    osf::EnemyEffectController controller;
    osf::RetailRandom random(1);
    std::vector<osf::WorldPosition> placements;
    if (!check(
            controller.initialize(request, &tables),
            "A shipped type-three subtype was rejected.")) {
        return false;
    }

    std::size_t actor_count = 0;
    std::size_t audio_count = 0;
    std::vector<std::int32_t> wave_x;
    for (std::int32_t update_number = 0;
         update_number < 22;
         ++update_number) {
        const osf::EnemyEffectControllerUpdate update =
            controller.update({
                {true, {7, 9}},
                &random,
                [&placements](
                    osf::WorldPosition position,
                    const osf::ObjectBounds& judgement) {
                    placements.push_back(position);
                    return judgement.left == -100 &&
                           judgement.top == -100 &&
                           judgement.right == 100 &&
                           judgement.bottom == 100;
                },
                {},
                {},
            });
        actor_count += update.actor_spawn_count;
        audio_count += update.audio_count;
        if (update.actor_spawn_count != 0) {
            wave_x.push_back(
                update.actor_spawns[0].position.x);
        }
        if (update_number == 2) {
            const auto& damaging =
                update.actor_spawns[0];
            const auto& second =
                update.actor_spawns[1];
            const auto& third =
                update.actor_spawns[2];
            if (!check(
                    update.actor_spawn_count == 3 &&
                        update.audio_count == 1 &&
                        update.audio[0].sample == 21 &&
                        update.audio[0].position.x == 1250 &&
                        update.audio[0].position.y == 2000 &&
                        damaging.resource_id == 10000030 &&
                        damaging.owner_kind == 4 &&
                        damaging.source_character_number ==
                            14000042 &&
                        damaging.target_mask == 19 &&
                        damaging.target_identifier == 0 &&
                        damaging.judgement.left == -100 &&
                        damaging.judgement.top == -100 &&
                        damaging.judgement.right == 100 &&
                        damaging.judgement.bottom == 100 &&
                        damaging.lifetime_from_animation &&
                        damaging.target_collision_start == 0 &&
                        damaging.target_collision_end == 0 &&
                        damaging.process_every_target &&
                        !damaging.expire_on_target &&
                        damaging.animation_chart == 1 &&
                        damaging.animation_direction == 8 &&
                        damaging.has_packet &&
                        damaging.packet[34] == 21013 &&
                        second.resource_id == 10000031 &&
                        second.animation_chart == 0 &&
                        second.target_collision_start == -1 &&
                        second.target_collision_end == 0 &&
                        !second.process_every_target &&
                        second.has_packet &&
                        third.resource_id == 10000032 &&
                        third.animation_chart == 0 &&
                        third.target_collision_start == -1 &&
                        third.target_collision_end == 0 &&
                        third.has_packet,
                    "The first type-three wave did not preserve "
                    "its three retail actor descriptors.")) {
                return false;
            }
        } else if (update_number < 2 &&
                   !check(
                       update.actor_spawn_count == 0 &&
                           update.audio_count == 0,
                       "Type three ignored its authored start "
                       "delay.")) {
            return false;
        }
    }

    if (!check(
            !controller.active() &&
                controller.counter() == 22 &&
                placements.size() == 5 &&
                actor_count == 15 &&
                audio_count == 5 &&
                wave_x ==
                    std::vector<std::int32_t>{
                        1250, 1450, 1650, 1850, 2050},
            "Type three did not use Table 205's five four-update "
            "waves and expanding radii.")) {
        return false;
    }

    controller.initialize(request, &tables);
    osf::RetailRandom blocked_random(1);
    std::size_t blocked_placement_count = 0;
    actor_count = 0;
    audio_count = 0;
    for (std::int32_t update_number = 0;
         update_number < 22;
         ++update_number) {
        const auto update = controller.update({
            {},
            &blocked_random,
            [&blocked_placement_count](
                osf::WorldPosition,
                const osf::ObjectBounds&) {
                ++blocked_placement_count;
                return false;
            },
            {},
            {},
        });
        actor_count += update.actor_spawn_count;
        audio_count += update.audio_count;
    }
    return check(
        !controller.active() &&
            blocked_placement_count == 5 &&
            actor_count == 0 &&
            audio_count == 0 &&
            blocked_random.state() == 1,
        "A blocked type-three placement did not suppress that "
        "wave and every later wave without consuming rand().");
#else
    return true;
#endif
}

bool testTypeTenWaves() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    osf::TableDatabase tables;
    std::string error;
    if (!check(
            tables.load(
                std::filesystem::path(
                    OPENSHADOWFLARE_SOURCE_DIR) /
                    "tmp" / "ShadowFlare" / "System" /
                    "Game" / "Parameter" / "Table.Tbd",
                &error),
            "The retail type-ten wave table could not be "
            "loaded.")) {
        std::cerr << error << '\n';
        return false;
    }
    const osf::TableData* wave_table = tables.find(206);
    const std::int32_t wave_count =
        wave_table ? wave_table->value(0, 19) : 0;
    if (!check(
            wave_count > 0,
            "Table 206 has no shipped type-ten subtype-20 "
            "wave count.")) {
        return false;
    }

    osf::CombatEffectSpawnRequest request =
        requestFor(10010, 2);
    request.has_explicit_origin = true;
    request.origin = {1000, 2000};
    request.constructor_value_17 = 20;
    request.target_kind = 19;
    request.target_identifier = -1;
    request.direction_radians = 0.0;

    osf::EnemyEffectController controller;
    osf::RetailRandom random(1);
    std::size_t placement_count = 0;
    std::size_t actor_count = 0;
    std::size_t audio_count = 0;
    std::size_t shake_count = 0;
    std::vector<std::int32_t> wave_updates;
    std::vector<std::int32_t> wave_x;
    if (!check(
            controller.initialize(request, &tables),
            "A shipped type-ten subtype was rejected.")) {
        return false;
    }

    const std::int32_t total_updates =
        2 + wave_count * 8;
    for (std::int32_t update_number = 0;
         update_number < total_updates;
         ++update_number) {
        const auto update = controller.update({
            {true, {7, 9}},
            &random,
            [&placement_count](
                osf::WorldPosition,
                const osf::ObjectBounds& judgement) {
                ++placement_count;
                return judgement.left == -150 &&
                       judgement.top == -150 &&
                       judgement.right == 150 &&
                       judgement.bottom == 150;
            },
            {true, {1250, 2000}},
            {},
        });
        actor_count += update.actor_spawn_count;
        audio_count += update.audio_count;
        shake_count += update.camera_shake ? 1u : 0u;

        const bool wave_update =
            update_number >= 2 &&
            (update_number - 2) % 8 == 0;
        if (!wave_update) {
            if (!check(
                    update.actor_spawn_count == 0 &&
                        update.audio_count == 0 &&
                        !update.camera_shake,
                    "Type ten emitted work between its "
                    "eight-update waves.")) {
                return false;
            }
            continue;
        }

        wave_updates.push_back(update_number);
        const std::int32_t wave_index =
            (update_number - 2) / 8;
        const std::int32_t expected_x =
            1250 + wave_index * 300;
        const auto& actor = update.actor_spawns[0];
        wave_x.push_back(actor.position.x);
        if (!check(
                update.actor_spawn_count == 1 &&
                    update.audio_count == 1 &&
                    update.audio[0].sample == 22 &&
                    update.audio[0].position.x ==
                        expected_x &&
                    update.audio[0].position.y == 2000 &&
                    actor.controller_effect_number == 10010 &&
                    actor.resource_id == 10000060 &&
                    actor.owner_kind == 4 &&
                    actor.source_character_number ==
                        14000042 &&
                    actor.target_mask == 19 &&
                    actor.target_identifier == 0 &&
                    actor.position.x == expected_x &&
                    actor.position.y == 2000 &&
                    actor.judgement.left == -150 &&
                    actor.judgement.top == -150 &&
                    actor.judgement.right == 150 &&
                    actor.judgement.bottom == 150 &&
                    actor.lifetime_from_animation &&
                    actor.target_collision_start == 0 &&
                    actor.target_collision_end == 0 &&
                    actor.process_every_target &&
                    !actor.expire_on_target &&
                    actor.animation_chart == 0 &&
                    actor.animation_direction == 8 &&
                    actor.has_packet &&
                    actor.packet[34] == 21013 &&
                    update.camera_shake ==
                        (expected_x - 1250 < 3001) &&
                    (!update.camera_shake ||
                     (update.camera_shake_duration == 8 &&
                      update.camera_shake_magnitude == 6)),
                "A type-ten wave differs from its retail actor, "
                "audio, placement, or camera descriptor.")) {
            return false;
        }
    }

    if (!check(
            !controller.active() &&
                controller.counter() == total_updates &&
                placement_count ==
                    static_cast<std::size_t>(wave_count) &&
                actor_count ==
                    static_cast<std::size_t>(wave_count) &&
                audio_count ==
                    static_cast<std::size_t>(wave_count) &&
                shake_count ==
                    static_cast<std::size_t>(
                        std::min(wave_count, 11)) &&
                random.state() == 1 &&
                wave_updates.size() ==
                    static_cast<std::size_t>(wave_count) &&
                wave_x.front() == 1250 &&
                wave_x.back() ==
                    1250 + (wave_count - 1) * 300,
            "Type ten did not use Table 206's delayed "
            "eight-update advancing wave sequence.")) {
        return false;
    }

    controller.initialize(request, &tables);
    std::size_t blocked_placement_count = 0;
    actor_count = 0;
    audio_count = 0;
    shake_count = 0;
    for (std::int32_t update_number = 0;
         update_number < total_updates;
         ++update_number) {
        const auto update = controller.update({
            {},
            &random,
            [&blocked_placement_count](
                osf::WorldPosition,
                const osf::ObjectBounds&) {
                ++blocked_placement_count;
                return false;
            },
            {true, {1250, 2000}},
            {},
        });
        actor_count += update.actor_spawn_count;
        audio_count += update.audio_count;
        shake_count += update.camera_shake ? 1u : 0u;
    }
    return check(
        !controller.active() &&
            blocked_placement_count ==
                static_cast<std::size_t>(wave_count) &&
            actor_count == 0 &&
            audio_count == 0 &&
            shake_count == 0 &&
            random.state() == 1,
        "A blocked type-ten placement did not permanently "
        "suppress its current and later waves.");
#else
    return true;
#endif
}

bool testTypeElevenRadialActors() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    osf::TableDatabase tables;
    std::string error;
    if (!check(
            tables.load(
                std::filesystem::path(
                    OPENSHADOWFLARE_SOURCE_DIR) /
                    "tmp" / "ShadowFlare" / "System" /
                    "Game" / "Parameter" / "Table.Tbd",
                &error),
            "The retail type-eleven count table could not be "
            "loaded.")) {
        std::cerr << error << '\n';
        return false;
    }
    const osf::TableData* count_table =
        tables.find(204);
    const std::int32_t actor_count =
        count_table ? count_table->value(0, 9) : 0;
    if (!check(
            actor_count == 3,
            "Table 204's shipped subtype-ten radial count "
            "changed.")) {
        return false;
    }

    osf::CombatEffectSpawnRequest request =
        requestFor(10011, 2);
    request.constructor_value_17 = 10;
    request.target_kind = 1;
    request.target_identifier = 3;
    request.direction_radians = 0.25;

    osf::EnemyEffectController controller;
    osf::EnemyEffectController missing_tables;
    if (!check(
            controller.initialize(request, &tables) &&
                !missing_tables.initialize(request),
            "Type eleven did not require and accept its shipped "
            "Table 204 entry.")) {
        return false;
    }

    const auto first = updateController(
        controller, {true, {100, 200}});
    if (!check(
            first.actor_spawn_count == 1 &&
                first.audio_count == 0 &&
                !first.expired &&
                first.actor_spawns[0].resource_id ==
                    10000012 &&
                first.actor_spawns[0].position.x == 100 &&
                first.actor_spawns[0].position.y == 200 &&
                first.actor_spawns[0]
                    .lifetime_from_animation,
            "Type eleven did not emit its source animation on "
            "update zero.")) {
        return false;
    }
    const auto second = updateController(
        controller, {true, {200, 300}});
    if (!check(
            second.actor_spawn_count == 0 &&
                second.audio_count == 0 &&
                !second.expired,
            "Type eleven ignored its authored radial delay.")) {
        return false;
    }

    const osf::WorldPosition source{300, 400};
    const auto burst =
        updateController(
            controller, {true, source});
    if (!check(
            burst.actor_spawn_count ==
                static_cast<std::size_t>(actor_count) &&
                burst.audio_count == 1 &&
                burst.expired &&
                !controller.active(),
            "Type eleven did not emit and expire its complete "
            "Table 204 radial burst.")) {
        return false;
    }

    osf::WorldPosition last_position;
    for (std::int32_t index = 0;
         index < actor_count;
         ++index) {
        const double direction =
            request.direction_radians -
            static_cast<double>(index) *
                (osf::kRetailFullCircleRadians /
                 static_cast<double>(actor_count));
        const osf::WorldPosition expected{
            source.x +
                static_cast<std::int32_t>(
                    std::cos(direction) * 180),
            source.y -
                static_cast<std::int32_t>(
                    std::sin(direction) * 180),
        };
        last_position = expected;
        const auto& actor =
            burst.actor_spawns[
                static_cast<std::size_t>(index)];
        if (!check(
                actor.controller_effect_number == 10011 &&
                    actor.resource_id == 10000010 &&
                    actor.owner_kind == 4 &&
                    actor.source_character_number ==
                        14000042 &&
                    actor.target_mask == 1 &&
                    actor.target_identifier == 3 &&
                    actor.home_toward_target &&
                    actor.homing_turn_speed == 20 &&
                    std::abs(
                        actor.direction_radians -
                        direction) < 0.0000001 &&
                    actor.travel_speed == 73 &&
                    actor.position.x == expected.x &&
                    actor.position.y == expected.y &&
                    actor.judgement.left == -80 &&
                    actor.judgement.top == -80 &&
                    actor.judgement.right == 79 &&
                    actor.judgement.bottom == 79 &&
                    actor.display_height == 250 &&
                    actor.lifetime == 90 &&
                    !actor.lifetime_from_animation &&
                    actor.expire_on_environment_collision &&
                    actor.target_collision_start == 0 &&
                    actor.target_collision_end == -1 &&
                    actor.expire_on_target &&
                    !actor.remember_targets &&
                    actor.target_audio.bank == 0 &&
                    actor.target_audio.sample == 20 &&
                    actor.animation_chart == 0 &&
                    actor.animation_direction ==
                        osf::retailDirectionForAngle(
                            direction) &&
                    actor.has_packet &&
                    actor.packet[34] == 21013,
                "A type-eleven radial child differs from its "
                "retail homing descriptor.")) {
            return false;
        }
    }
    if (!check(
            burst.audio[0].sample == 19 &&
                burst.audio[0].position.x ==
                    last_position.x &&
                burst.audio[0].position.y ==
                    last_position.y,
            "Type eleven did not place its single sample 19 at "
            "the last radial child.")) {
        return false;
    }

    request.owner_kind = 0;
    request.has_explicit_origin = true;
    request.origin = {700, 900};
    request.constructor_value_12 = 0;
    controller.initialize(request, &tables);
    const auto fixed =
        updateController(
            controller, {true, {1, 2}});
    if (!check(
            fixed.actor_spawn_count == 4 &&
                fixed.actor_spawns[0].position.x == 700 &&
                fixed.actor_spawns[0].position.y == 900,
            "A zero-owner type-eleven burst lost its fixed "
            "source actor.")) {
        return false;
    }
    for (std::size_t index = 1;
         index < fixed.actor_spawn_count;
         ++index) {
        if (!check(
                fixed.actor_spawns[index].position.x == 700 &&
                    fixed.actor_spawns[index].position.y == 900,
                "A zero-owner type-eleven radial child was "
                "incorrectly projected 180 units.")) {
            return false;
        }
    }

    request.constructor_value_17 = 30;
    controller.initialize(request, &tables);
    const auto maximum =
        updateController(controller, {});
    return check(
        maximum.actor_spawn_count == 9 &&
            maximum.audio_count == 1 &&
            maximum.expired,
        "Type eleven did not retain the source plus all eight "
        "Table 204 subtype-thirty radial actors.");
#else
    return true;
#endif
}

bool testTypeTwelveWarningAndProjectileFans() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    osf::TableDatabase tables;
    std::string error;
    if (!check(
            tables.load(
                std::filesystem::path(
                    OPENSHADOWFLARE_SOURCE_DIR) /
                    "tmp" / "ShadowFlare" / "System" /
                    "Game" / "Parameter" / "Table.Tbd",
                &error),
            "The retail type-twelve count table could not be "
            "loaded.")) {
        std::cerr << error << '\n';
        return false;
    }
    const osf::TableData* count_table =
        tables.find(204);
    const std::int32_t actor_count =
        count_table ? count_table->value(0, 9) : 0;
    const std::int32_t spread_divisor =
        count_table ? count_table->value(0, 29) : 0;
    if (!check(
            actor_count == 3 &&
                spread_divisor == 8,
            "Table 204's type-twelve count or spread divisor "
            "changed.")) {
        return false;
    }

    osf::CombatEffectSpawnRequest request =
        requestFor(10012, 2);
    request.constructor_value_17 = 10;
    request.constructor_value_22 = 1;
    request.direction_radians = 0.25;
    request.packet.write(35, 8);
    request.packet.write(74, -1);
    request.packet.write(75, 8);

    const auto direction_for =
        [&request, actor_count, spread_divisor](
            std::int32_t index) {
            const double spread =
                static_cast<double>(actor_count) *
                2.5132736 /
                static_cast<double>(spread_divisor);
            return request.direction_radians -
                   spread * 0.5 +
                   static_cast<double>(
                       1 - (actor_count & 1)) *
                       (spread /
                        static_cast<double>(actor_count)) *
                       0.5 +
                   static_cast<double>(index) * spread /
                       static_cast<double>(actor_count - 1);
        };

    osf::EnemyEffectController controller;
    osf::EnemyEffectController missing_tables;
    if (!check(
            controller.initialize(request, &tables) &&
                !missing_tables.initialize(request),
            "Type twelve did not require and accept its shipped "
            "Table 204 row.")) {
        return false;
    }

    const osf::WorldPosition first_source{100, 200};
    const auto first =
        updateController(
            controller, {true, first_source});
    if (!check(
            first.actor_spawn_count == 4 &&
                first.audio_count == 0 &&
                !first.expired &&
                first.actor_spawns[0].resource_id ==
                    11000027 &&
                first.actor_spawns[0].position.x == 100 &&
                first.actor_spawns[0].position.y == 200 &&
                first.actor_spawns[0]
                    .lifetime_from_animation,
            "Type twelve did not create its source and complete "
            "warning fan on update zero.")) {
        return false;
    }
    for (std::int32_t index = 0;
         index < actor_count;
         ++index) {
        const double direction = direction_for(index);
        const osf::WorldPosition expected{
            first_source.x +
                static_cast<std::int32_t>(
                    std::cos(direction) * 150),
            first_source.y -
                static_cast<std::int32_t>(
                    std::sin(direction) * 150),
        };
        const auto& actor =
            first.actor_spawns[
                static_cast<std::size_t>(index + 1)];
        if (!check(
                actor.controller_effect_number == 10012 &&
                    actor.resource_id == 10000080 &&
                    actor.owner_kind == 4 &&
                    actor.source_character_number ==
                        14000042 &&
                    actor.target_mask == 1 &&
                    actor.target_identifier == 3 &&
                    std::abs(
                        actor.direction_radians -
                        direction) < 0.0000001 &&
                    actor.travel_speed == 0 &&
                    actor.position.x == expected.x &&
                    actor.position.y == expected.y &&
                    actor.judgement.left == -50 &&
                    actor.judgement.top == -50 &&
                    actor.judgement.right == 50 &&
                    actor.judgement.bottom == 50 &&
                    actor.display_height == 250 &&
                    actor.lifetime == 10 &&
                    !actor.lifetime_from_animation &&
                    !actor.expire_on_environment_collision &&
                    actor.target_collision_start == -1 &&
                    actor.animation_chart == 0 &&
                    actor.animation_direction ==
                        osf::retailDirectionForAngle(
                            direction) &&
                    actor.has_packet &&
                    actor.packet[34] == 21013 &&
                    actor.packet[35] == 8 &&
                    actor.packet[74] == -1 &&
                    actor.packet[75] == 8,
                "A type-twelve warning differs from its retail "
                "stationary descriptor.")) {
            return false;
        }
    }

    const auto second =
        updateController(
            controller, {true, {200, 300}});
    if (!check(
            second.actor_spawn_count == 0 &&
                second.audio_count == 0 &&
                !second.expired,
            "Type twelve launched before its authored delay.")) {
        return false;
    }

    const osf::WorldPosition launch_source{300, 400};
    const auto launch =
        updateController(
            controller, {true, launch_source});
    if (!check(
            launch.actor_spawn_count ==
                static_cast<std::size_t>(actor_count) &&
                launch.audio_count == 1 &&
                launch.expired &&
                !controller.active(),
            "Type twelve did not replace its warning fan with "
            "the complete projectile fan.")) {
        return false;
    }

    osf::WorldPosition last_position;
    for (std::int32_t index = 0;
         index < actor_count;
         ++index) {
        const double direction = direction_for(index);
        const std::int32_t animation_direction =
            osf::retailDirectionForAngle(direction);
        const osf::WorldPosition expected{
            launch_source.x +
                static_cast<std::int32_t>(
                    std::cos(direction) * 180),
            launch_source.y -
                static_cast<std::int32_t>(
                    std::sin(direction) * 180),
        };
        last_position = expected;
        const auto& actor =
            launch.actor_spawns[
                static_cast<std::size_t>(index)];
        if (!check(
                actor.controller_effect_number == 10012 &&
                    actor.resource_id == 10000081 &&
                    actor.owner_kind == 4 &&
                    actor.source_character_number ==
                        14000042 &&
                    actor.target_mask == 1 &&
                    actor.target_identifier == 3 &&
                    !actor.home_toward_target &&
                    std::abs(
                        actor.direction_radians -
                        direction) < 0.0000001 &&
                    actor.travel_speed == 73 &&
                    actor.position.x == expected.x &&
                    actor.position.y == expected.y &&
                    actor.judgement.left == -50 &&
                    actor.judgement.top == -50 &&
                    actor.judgement.right == 50 &&
                    actor.judgement.bottom == 50 &&
                    actor.display_height == 250 &&
                    actor.lifetime == 90 &&
                    actor.expire_on_environment_collision &&
                    actor.target_collision_start == 0 &&
                    actor.target_collision_end == -1 &&
                    actor.expire_on_target &&
                    actor.remember_targets &&
                    actor.target_audio.bank == 0 &&
                    actor.target_audio.sample == 20 &&
                    actor.animation_chart == 0 &&
                    actor.animation_direction ==
                        animation_direction &&
                    actor.has_packet &&
                    actor.packet[34] == 21021 &&
                    actor.packet[35] ==
                        animation_direction &&
                    actor.packet[74] == 21022 &&
                    actor.packet[75] ==
                        animation_direction,
                "A type-twelve projectile differs from its "
                "retail moving descriptor or packet rewrite.")) {
            return false;
        }
    }
    if (!check(
            launch.audio[0].sample == 94 &&
                launch.audio[0].position.x ==
                    last_position.x &&
                launch.audio[0].position.y ==
                    last_position.y,
            "Type twelve did not place sample 94 at its last "
            "projectile.")) {
        return false;
    }

    request.owner_kind = 0;
    request.has_explicit_origin = true;
    request.origin = {700, 900};
    request.constructor_value_12 = 0;
    controller.initialize(request, &tables);
    const auto fixed =
        updateController(
            controller, {true, {1, 2}});
    if (!check(
            fixed.actor_spawn_count == 7 &&
                fixed.audio_count == 1 &&
                fixed.expired,
            "A zero-delay type-twelve controller lost one of "
            "its source, warning, or projectile actors.")) {
        return false;
    }
    for (std::size_t index = 0;
         index < fixed.actor_spawn_count;
         ++index) {
        if (!check(
                fixed.actor_spawns[index].position.x == 700 &&
                    fixed.actor_spawns[index].position.y == 900,
                "A zero-owner type-twelve actor was incorrectly "
                "projected away from its fixed origin.")) {
            return false;
        }
    }

    request.constructor_value_17 = 30;
    controller.initialize(request, &tables);
    const auto maximum =
        updateController(controller, {});
    const double maximum_spread =
        8.0 * 2.5132736 / 8.0;
    const double maximum_first_direction =
        request.direction_radians -
        maximum_spread * 0.5 +
        maximum_spread / 8.0 * 0.5;
    const double maximum_last_direction =
        maximum_first_direction +
        maximum_spread;
    return check(
        maximum.actor_spawn_count == 17 &&
            maximum.audio_count == 1 &&
            maximum.expired &&
            std::abs(
                maximum.actor_spawns[1]
                        .direction_radians -
                    maximum_first_direction) <
                0.0000001 &&
            std::abs(
                maximum.actor_spawns[8]
                        .direction_radians -
                    maximum_last_direction) <
                0.0000001 &&
            std::abs(
                maximum.actor_spawns[9]
                        .direction_radians -
                    maximum_first_direction) <
                0.0000001 &&
            std::abs(
                maximum.actor_spawns[16]
                        .direction_radians -
                    maximum_last_direction) <
                0.0000001,
        "Type twelve did not retain the source plus both "
        "eight-actor Table 204 fans and its even-count angle "
        "offset.");
#else
    return true;
#endif
}

bool testTypeThirteenRadialWaves() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    osf::TableDatabase tables;
    std::string error;
    if (!check(
            tables.load(
                std::filesystem::path(
                    OPENSHADOWFLARE_SOURCE_DIR) /
                    "tmp" / "ShadowFlare" / "System" /
                    "Game" / "Parameter" / "Table.Tbd",
                &error),
            "The retail type-thirteen count table could not be "
            "loaded.")) {
        std::cerr << error << '\n';
        return false;
    }
    const osf::TableData* count_table =
        tables.find(204);
    const std::int32_t actor_count =
        count_table ? count_table->value(0, 9) : 0;
    if (!check(
            actor_count == 3,
            "Table 204's shipped subtype-ten type-thirteen "
            "radial count changed.")) {
        return false;
    }

    osf::CombatEffectSpawnRequest request =
        requestFor(10013, 2);
    request.constructor_value_17 = 10;
    request.direction_radians = 0.25;
    request.has_explicit_origin = true;
    request.origin = {1000, 2000};

    osf::EnemyEffectController controller;
    osf::EnemyEffectController missing_tables;
    if (!check(
            controller.initialize(request, &tables) &&
                !missing_tables.initialize(request),
            "Type thirteen did not require and accept its "
            "shipped Table 204 entry.")) {
        return false;
    }

    osf::RetailRandom random(1);
    osf::RetailRandom expected_random(1);
    std::size_t placement_count = 0;
    const double angle_step =
        osf::kRetailFullCircleRadians /
        static_cast<double>(actor_count);
    for (std::int32_t update_number = 0;
         update_number < 18;
         ++update_number) {
        const bool pulse =
            update_number >= 2 &&
            (update_number - 2) % 4 == 0;
        const std::int32_t wave =
            pulse ? (update_number - 2) / 4 : -1;
        const auto update = controller.update(
            {
                {},
                &random,
                [&placement_count](
                    osf::WorldPosition,
                    const osf::ObjectBounds& judgement) {
                    ++placement_count;
                    return judgement.left == -100 &&
                           judgement.top == -100 &&
                           judgement.right == 100 &&
                           judgement.bottom == 100;
                },
                {},
                {},
            });
        if (!check(
                update.actor_spawn_count ==
                        (pulse ? 9u : 0u) &&
                    update.audio_count ==
                        (pulse ? 1u : 0u) &&
                    update.expired ==
                        (update_number == 17),
                "Type thirteen lost its delay, four-update "
                "pulse cadence, or delay-plus-sixteen "
                "lifetime.")) {
            return false;
        }
        if (!pulse) {
            continue;
        }

        const std::int32_t radius =
            wave * 200 + 350;
        osf::WorldPosition last_position;
        for (std::int32_t index = 0;
             index < actor_count;
             ++index) {
            const double direction =
                request.direction_radians +
                static_cast<double>(index) *
                    angle_step;
            const osf::WorldPosition expected{
                request.origin.x +
                    static_cast<std::int32_t>(
                        std::cos(direction) * radius),
                request.origin.y -
                    static_cast<std::int32_t>(
                        std::sin(direction) * radius),
            };
            last_position = expected;
            const std::int32_t expected_chart =
                expected_random.next() % 4;
            const std::size_t base =
                static_cast<std::size_t>(index) * 3;
            const auto& damaging =
                update.actor_spawns[base];
            const auto& second =
                update.actor_spawns[base + 1];
            const auto& third =
                update.actor_spawns[base + 2];
            if (!check(
                    damaging.controller_effect_number ==
                            10013 &&
                        damaging.resource_id == 10000030 &&
                        damaging.owner_kind == 4 &&
                        damaging.source_character_number ==
                            14000042 &&
                        damaging.target_mask == 1 &&
                        damaging.target_identifier == 0 &&
                        damaging.position.x == expected.x &&
                        damaging.position.y == expected.y &&
                        damaging.judgement.left == -100 &&
                        damaging.judgement.top == -100 &&
                        damaging.judgement.right == 100 &&
                        damaging.judgement.bottom == 100 &&
                        damaging.lifetime_from_animation &&
                        damaging.target_collision_start == 0 &&
                        damaging.target_collision_end == 0 &&
                        damaging.process_every_target &&
                        damaging.animation_chart ==
                            expected_chart &&
                        damaging.animation_direction == 8 &&
                        damaging.has_packet &&
                        damaging.packet[34] == 21013 &&
                        second.resource_id == 10000031 &&
                        second.position.x == expected.x &&
                        second.position.y == expected.y &&
                        second.lifetime_from_animation &&
                        second.target_collision_start == -1 &&
                        !second.process_every_target &&
                        second.animation_chart == 0 &&
                        second.animation_direction == 8 &&
                        second.has_packet &&
                        third.resource_id == 10000032 &&
                        third.position.x == expected.x &&
                        third.position.y == expected.y &&
                        third.lifetime_from_animation &&
                        third.target_collision_start == -1 &&
                        !third.process_every_target &&
                        third.animation_chart == 0 &&
                        third.animation_direction == 8 &&
                        third.has_packet,
                    "A type-thirteen radial point differs from "
                    "its retail three-layer descriptor.")) {
                return false;
            }
        }
        if (!check(
                update.audio[0].sample == 21 &&
                    update.audio[0].position.x ==
                        last_position.x &&
                    update.audio[0].position.y ==
                        last_position.y,
                "Type thirteen did not place sample 21 at the "
                "last point of its radial shell.")) {
            return false;
        }
    }
    if (!check(
            !controller.active() &&
                controller.counter() == 18 &&
                placement_count == 12 &&
                random.state() == expected_random.state(),
            "Type thirteen did not test every radial point or "
            "consume one random chart per clear point.")) {
        return false;
    }

    request.constructor_value_12 = 0;
    osf::EnemyEffectController blocked;
    blocked.initialize(request, &tables);
    osf::RetailRandom blocked_random(1);
    osf::RetailRandom blocked_expected(1);
    std::size_t blocked_placement_count = 0;
    std::size_t blocked_actor_count = 0;
    std::size_t blocked_audio_count = 0;
    for (std::int32_t update_number = 0;
         update_number < 16;
         ++update_number) {
        const auto update = blocked.update(
            {
                {},
                &blocked_random,
                [&blocked_placement_count, actor_count](
                    osf::WorldPosition,
                    const osf::ObjectBounds&) {
                    const std::size_t call =
                        blocked_placement_count++;
                    const std::size_t wave =
                        call /
                        static_cast<std::size_t>(actor_count);
                    const std::size_t ray =
                        call %
                        static_cast<std::size_t>(actor_count);
                    return !(wave >= 1 && ray == 0);
                },
                {},
                {},
            });
        blocked_actor_count += update.actor_spawn_count;
        blocked_audio_count += update.audio_count;
        if (update_number % 4 == 0) {
            const std::int32_t clear_rays =
                update_number == 0 ? 3 : 2;
            for (std::int32_t index = 0;
                 index < clear_rays;
                 ++index) {
                blocked_expected.next();
            }
        }
    }
    if (!check(
            !blocked.active() &&
                blocked_placement_count == 12 &&
                blocked_actor_count == 27 &&
                blocked_audio_count == 4 &&
                blocked_random.state() ==
                    blocked_expected.state(),
            "A blocked type-thirteen ray did not latch "
            "independently while the other rays and shell "
            "sounds continued.")) {
        return false;
    }

    request.constructor_value_17 = 30;
    osf::EnemyEffectController maximum;
    if (!maximum.initialize(request, &tables)) {
        return false;
    }
    const auto maximum_update =
        maximum.update({{}, nullptr, {}, {}, {}});
    return check(
        maximum_update.actor_spawn_count == 24 &&
            maximum_update.audio_count == 1 &&
            !maximum_update.expired,
        "Type thirteen did not retain all eight Table 204 rays "
        "within one three-layer shell.");
#else
    return true;
#endif
}

bool testTypeFourteenProjectile() {
    osf::CombatEffectSpawnRequest request =
        requestFor(10014, 2);
    request.constructor_value_22 = 1;
    request.direction_radians =
        3.14159265358979323846 / 2.0;

    osf::EnemyEffectController controller;
    if (!check(
            controller.initialize(request),
            "A valid type-fourteen request was rejected.")) {
        return false;
    }
    const auto first =
        updateController(
            controller, {true, {100, 200}});
    const auto second =
        updateController(
            controller, {true, {200, 300}});
    if (!check(
            first.actor_spawn_count == 0 &&
                first.audio_count == 0 &&
                !first.expired &&
                second.actor_spawn_count == 0 &&
                second.audio_count == 0 &&
                !second.expired,
            "Type fourteen created a source actor or launched "
            "before its authored delay.")) {
        return false;
    }

    const auto launch =
        updateController(
            controller, {true, {300, 400}});
    if (!check(
            launch.actor_spawn_count == 1 &&
                launch.audio_count == 1 &&
                launch.expired &&
                !controller.active(),
            "Type fourteen did not launch once and immediately "
            "remove its controller.")) {
        return false;
    }
    const auto& actor = launch.actor_spawns[0];
    if (!check(
            actor.controller_effect_number == 10014 &&
                actor.resource_id == 10000070 &&
                actor.owner_kind == 4 &&
                actor.source_character_number ==
                    14000042 &&
                actor.target_mask == 1 &&
                actor.target_identifier == 3 &&
                actor.travel_speed == 73 &&
                actor.display_height == 250 &&
                actor.direction_radians ==
                    request.direction_radians &&
                actor.position.x == 300 &&
                actor.position.y == 220 &&
                actor.judgement.left == -50 &&
                actor.judgement.top == -50 &&
                actor.judgement.right == 50 &&
                actor.judgement.bottom == 50 &&
                actor.lifetime == -1 &&
                !actor.lifetime_from_animation &&
                actor.expire_on_environment_collision &&
                actor.target_collision_start == 0 &&
                actor.target_collision_end == -1 &&
                actor.expire_on_target &&
                actor.remember_targets &&
                actor.target_audio.bank == 0 &&
                actor.target_audio.sample == 20 &&
                actor.animation_chart == 0 &&
                actor.animation_direction == 3 &&
                actor.has_packet &&
                actor.packet[2] == 14000042 &&
                actor.packet[34] == 21013,
            "The type-fourteen projectile differs from its "
            "retail moving descriptor.")) {
        return false;
    }
    if (!check(
            launch.audio[0].sample == 22 &&
                launch.audio[0].position.x == 300 &&
                launch.audio[0].position.y == 220,
            "Type fourteen did not place sample 22 at its "
            "projectile.")) {
        return false;
    }

    request.owner_kind = 0;
    request.has_explicit_origin = true;
    request.origin = {700, 900};
    request.constructor_value_12 = 0;
    controller.initialize(request);
    const auto fixed =
        updateController(
            controller, {true, {1, 2}});
    return check(
        fixed.actor_spawn_count == 1 &&
            fixed.audio_count == 1 &&
            fixed.expired &&
            fixed.actor_spawns[0].position.x == 700 &&
            fixed.actor_spawns[0].position.y == 900 &&
            fixed.audio[0].position.x == 700 &&
            fixed.audio[0].position.y == 900,
        "A fixed-origin type-fourteen projectile was "
        "incorrectly projected 180 units.");
}

bool testTypeSixteenProjectileExplosion() {
    osf::CombatEffectSpawnRequest request =
        requestFor(10016, 2);
    request.constructor_value_22 = 1;

    osf::EnemyEffectController controller;
    if (!check(
            controller.initialize(request),
            "A valid type-sixteen request was rejected.")) {
        return false;
    }
    const auto first =
        updateController(
            controller, {true, {100, 200}});
    const auto second =
        updateController(
            controller, {true, {200, 300}});
    if (!check(
            first.actor_spawn_count == 0 &&
                first.audio_count == 0 &&
                second.actor_spawn_count == 0 &&
                second.audio_count == 0 &&
                controller.counter() == 2,
            "Type sixteen created a source actor or launched "
            "before its authored delay.")) {
        return false;
    }

    const auto launch =
        updateController(
            controller, {true, {300, 400}});
    if (!check(
            launch.actor_spawn_count == 1 &&
                launch.audio_count == 1 &&
                !launch.expired &&
                controller.active() &&
                controller.counter() == 3,
            "Type sixteen did not launch one tracked projectile "
            "while retaining its controller.")) {
        return false;
    }
    const auto& projectile =
        launch.actor_spawns[0];
    if (!check(
            projectile.controller_effect_number == 10016 &&
                projectile.resource_id == 10000110 &&
                projectile.owner_kind == 4 &&
                projectile.source_character_number ==
                    14000042 &&
                projectile.target_mask == 1 &&
                projectile.target_identifier == 3 &&
                projectile.direction_radians == 0.0 &&
                projectile.travel_speed == 73 &&
                projectile.position.x == 480 &&
                projectile.position.y == 400 &&
                projectile.judgement.left == -80 &&
                projectile.judgement.top == -80 &&
                projectile.judgement.right == 79 &&
                projectile.judgement.bottom == 79 &&
                projectile.display_height == 250 &&
                projectile.lifetime == -1 &&
                !projectile.lifetime_from_animation &&
                projectile.expire_on_environment_collision &&
                projectile.target_collision_start == 0 &&
                projectile.target_collision_end == -1 &&
                projectile.expire_on_target &&
                projectile.remember_targets &&
                projectile.target_audio.bank == 0 &&
                projectile.target_audio.sample == 20 &&
                projectile.animation_chart == 0 &&
                projectile.animation_direction == 1 &&
                projectile.track_for_controller &&
                projectile.has_packet &&
                projectile.packet[34] == 21013 &&
                launch.audio[0].sample == 19 &&
                launch.audio[0].position.x == 480 &&
                launch.audio[0].position.y == 400,
            "The type-sixteen projectile differs from its "
            "retail tracked moving descriptor.")) {
        return false;
    }

    controller.bindSpawnedActor(
        50000042, {true, {480, 400}});
    std::int32_t resolver_calls = 0;
    osf::EnemyEffectControllerContext context;
    context.resolve_actor =
        [&resolver_calls](
            std::int32_t actor_identifier) {
            ++resolver_calls;
            return actor_identifier == 50000042
                ? osf::EnemyEffectControllerSource{
                      true, {600, 700}}
                : osf::EnemyEffectControllerSource{};
        };
    const auto tracked = controller.update(context);
    if (!check(
            tracked.actor_spawn_count == 0 &&
                tracked.audio_count == 0 &&
                !tracked.expired &&
                controller.counter() == 4 &&
                resolver_calls == 1,
            "Type sixteen did not retain the live projectile's "
            "latest position.")) {
        return false;
    }

    context.resolve_actor =
        [&resolver_calls](std::int32_t actor_identifier) {
            ++resolver_calls;
            return osf::EnemyEffectControllerSource{
                actor_identifier != 50000042, {}};
        };
    context.observer = {true, {3600, 700}};
    const auto explosion = controller.update(context);
    if (!check(
            explosion.actor_spawn_count == 1 &&
                explosion.audio_count == 1 &&
                explosion.camera_shake &&
                explosion.camera_shake_duration == 8 &&
                explosion.camera_shake_magnitude == 6 &&
                explosion.expired &&
                !controller.active() &&
                resolver_calls == 2,
            "Type sixteen did not replace its missing "
            "projectile with the explosion, sound, and "
            "inclusive 3000-unit camera shake.")) {
        return false;
    }
    const auto& burst = explosion.actor_spawns[0];
    if (!check(
            burst.controller_effect_number == 10016 &&
                burst.resource_id == 10000111 &&
                burst.owner_kind == 4 &&
                burst.source_character_number == 14000042 &&
                burst.target_mask == 1 &&
                burst.target_identifier == 3 &&
                burst.travel_speed == 0 &&
                burst.position.x == 600 &&
                burst.position.y == 700 &&
                burst.judgement.left == -240 &&
                burst.judgement.top == -240 &&
                burst.judgement.right == 239 &&
                burst.judgement.bottom == 239 &&
                burst.display_height == 0 &&
                burst.lifetime_from_animation &&
                burst.target_collision_start == 5 &&
                burst.target_collision_end == 5 &&
                burst.process_every_target &&
                !burst.expire_on_target &&
                burst.target_audio.bank == 0 &&
                burst.target_audio.sample == 20 &&
                burst.animation_chart == 0 &&
                burst.animation_direction == 8 &&
                !burst.track_for_controller &&
                burst.has_packet &&
                burst.packet[34] == 21013 &&
                explosion.audio[0].sample == 22 &&
                explosion.audio[0].position.x == 600 &&
                explosion.audio[0].position.y == 700,
            "The type-sixteen explosion differs from its "
            "retail delayed area descriptor.")) {
        return false;
    }

    request.owner_kind = 0;
    request.has_explicit_origin = true;
    request.origin = {700, 900};
    request.constructor_value_12 = 0;
    controller.initialize(request);
    const auto fixed =
        updateController(
            controller, {true, {1, 2}});
    controller.bindSpawnedActor(
        50000043, {true, {700, 900}});
    osf::EnemyEffectControllerContext far_context;
    far_context.resolve_actor =
        [](std::int32_t) {
            return osf::EnemyEffectControllerSource{};
        };
    far_context.observer = {true, {3701, 900}};
    const auto far = controller.update(far_context);
    return check(
        fixed.actor_spawn_count == 1 &&
            fixed.actor_spawns[0].position.x == 700 &&
            fixed.actor_spawns[0].position.y == 900 &&
            far.actor_spawn_count == 1 &&
            far.expired &&
            !far.camera_shake,
        "Type sixteen projected a fixed origin or shook the "
        "camera beyond retail's 3000-unit boundary.");
}

}  // namespace

int main() {
    if (!testTypeOneZeroDelay() ||
        !testTypeTwoDelayedReresolution() ||
        !testFixedOriginAndMissingSource() ||
        !testNegativeDelayAndRejectedRequests() ||
        !testTypeFourWarningBurstAndCameraShake() ||
        !testTypeFiveFrameCountSequence() ||
        !testTypeThreeWaves() ||
        !testTypeTenWaves() ||
        !testTypeElevenRadialActors() ||
        !testTypeTwelveWarningAndProjectileFans() ||
        !testTypeThirteenRadialWaves() ||
        !testTypeFourteenProjectile() ||
        !testTypeSixteenProjectileExplosion()) {
        return 1;
    }
    return 0;
}
