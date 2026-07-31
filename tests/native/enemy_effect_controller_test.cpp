#include "core/retail_random.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
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
    return controller.update({source, nullptr, {}, {}});
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
                fixed.actor_spawns[1].position.x == 880 &&
                fixed.actor_spawns[1].position.y == 900,
            "An owner-kind-zero controller ignored its stored "
            "origin.")) {
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
    request.effect_number = 10005;
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
        });
    }
    return check(
        far_burst.expired &&
            !far_burst.camera_shake,
        "Type four shook an observer outside the strict "
        "3001-unit retail range.");
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

}  // namespace

int main() {
    if (!testTypeOneZeroDelay() ||
        !testTypeTwoDelayedReresolution() ||
        !testFixedOriginAndMissingSource() ||
        !testNegativeDelayAndRejectedRequests() ||
        !testTypeFourWarningBurstAndCameraShake() ||
        !testTypeThreeWaves()) {
        return 1;
    }
    return 0;
}
