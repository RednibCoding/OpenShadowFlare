#include "world/enemy_effect_controller.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>

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
        controller.update({true, {100, 200}});
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
        controller.update({true, {100, 200}});
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
        controller.update({true, {200, 300}});
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
        controller.update({true, {300, 400}});
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
        controller.update({true, {1, 2}});
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
        controller.update({false, {600, 800}});
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
        controller.update({true, {700, 800}});
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
            controller.update({true, {10, 20}});
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
    request.effect_number = 10003;
    return check(
        !controller.initialize(request) &&
            controller.update({}).expired,
        "An unimplemented specialized family entered the "
        "type-one/type-two controller.");
}

}  // namespace

int main() {
    if (!testTypeOneZeroDelay() ||
        !testTypeTwoDelayedReresolution() ||
        !testFixedOriginAndMissingSource() ||
        !testNegativeDelayAndRejectedRequests()) {
        return 1;
    }
    return 0;
}
