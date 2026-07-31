#include "resources/effect_visual_resource.hpp"
#include "core/retail_random.hpp"
#include "world/enemy_effect_controller.hpp"
#include "world/movement_controller.hpp"
#include "world/runtime_effect_actor.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

void appendI16(
    std::vector<std::uint8_t>& bytes,
    std::int16_t value) {
    const std::uint16_t raw =
        static_cast<std::uint16_t>(value);
    bytes.push_back(static_cast<std::uint8_t>(raw));
    bytes.push_back(
        static_cast<std::uint8_t>(raw >> 8u));
}

void appendI32(
    std::vector<std::uint8_t>& bytes,
    std::int32_t value) {
    const std::uint32_t raw =
        static_cast<std::uint32_t>(value);
    bytes.push_back(static_cast<std::uint8_t>(raw));
    bytes.push_back(
        static_cast<std::uint8_t>(raw >> 8u));
    bytes.push_back(
        static_cast<std::uint8_t>(raw >> 16u));
    bytes.push_back(
        static_cast<std::uint8_t>(raw >> 24u));
}

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

osf::ObjectMap oneBlockingObject(
    std::int32_t x,
    std::int32_t y) {
    std::vector<std::uint8_t> bytes;
    const char header[] = "RPGSCRN_OBJv001\x1a";
    bytes.insert(bytes.end(), header, header + 16);
    appendI32(bytes, 1);
    appendI32(bytes, x);
    appendI32(bytes, y);
    appendI16(bytes, -1);
    appendI16(bytes, -1);
    appendI16(bytes, -1);
    appendI16(bytes, 1000);
    appendI16(bytes, 1);
    appendI16(bytes, 0);
    appendI16(bytes, 1000);
    appendI16(bytes, 1000);
    appendI16(bytes, 1000);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);

    osf::ObjectMap result;
    result.decode(bytes);
    return result;
}

osf::GroundMap oneSpecialBlockingGround() {
    std::vector<std::uint8_t> bytes;
    const char header[16] = "RPGSCRN_GNDv000";
    bytes.insert(
        bytes.end(), header, header + sizeof(header));
    appendI32(bytes, 2);
    appendI32(bytes, 1);
    appendI32(bytes, 64);
    appendI32(bytes, 48);
    appendI32(bytes, 160);
    appendI32(bytes, 160);
    bytes.push_back(0);
    for (std::int32_t index = 0;
         index < 6;
         ++index) {
        appendI16(bytes, 0);
    }
    bytes.push_back(0);
    for (std::int32_t index = 0;
         index < 36;
         ++index) {
        appendI16(bytes, index == 25 ? 3 : 0);
    }
    osf::GroundMap result;
    result.decode(bytes);
    return result;
}

std::filesystem::path optionDirectory(
    std::int32_t resource_id) {
    return std::filesystem::path(
               OPENSHADOWFLARE_SOURCE_DIR) /
           "tmp" / "ShadowFlare" / "Character" /
           "OPTION" /
           std::to_string(resource_id);
}

bool loadVisual(
    std::int32_t resource_id,
    osf::EffectVisualResource& visual) {
    std::string error;
    if (visual.load(
            optionDirectory(resource_id), &error)) {
        return true;
    }
    std::cerr << error << '\n';
    return false;
}

osf::CombatEffectSpawnRequest controllerRequest(
    std::int32_t effect_number,
    std::int32_t delay) {
    osf::CombatEffectSpawnRequest request;
    request.valid = true;
    request.effect_number = effect_number;
    request.owner_kind = 4;
    request.source_character_number = 14000042;
    request.target_kind = 19;
    request.target_identifier = -1;
    request.constructor_value_6 = 60;
    request.constructor_value_7 = 250;
    request.direction_radians = 0.0;
    request.has_source_judgement = true;
    request.source_judgement = {-20, -30, 21, 31};
    request.constructor_value_12 = delay;
    request.has_packet = true;
    request.packet.write(2, 14000042);
    return request;
}

bool testSourceAnimationLifetime() {
    osf::EnemyEffectController controller;
    controller.initialize(
        controllerRequest(10001, 4));
    const auto controller_update =
        controller.update({
            {true, {100, 200}}, nullptr, {}, {}});

    osf::EffectVisualResource visual;
    if (!loadVisual(10000012, visual)) {
        return false;
    }
    const std::int32_t frame_count =
        visual.animation()
            .charts()[0]
            .directions[8]
            .frame_count;
    osf::RuntimeEffectActor actor;
    if (!check(
            actor.initialize(
                controller_update.actor_spawns[0],
                visual) &&
                actor.lifetime() == frame_count &&
                actor.animationChart() == 0 &&
                actor.animationDirection() == 8 &&
                actor.displayHeight() == 0,
            "The source actor did not resolve its lifetime from "
            "retail chart zero, direction eight.")) {
        return false;
    }

    const osf::GroundMap ground;
    const osf::ObjectMap objects;
    for (std::int32_t update_number = 0;
         update_number < frame_count;
         ++update_number) {
        const osf::RuntimeEffectActorUpdate update =
            actor.update(ground, objects);
        if (!check(
                update.target_collision_active == false &&
                    actor.animationFrame() ==
                        update_number &&
                    update.expired ==
                        (update_number ==
                         frame_count - 1),
                "The source actor did not show each chart-zero "
                "frame for one retail update.")) {
            return false;
        }
    }
    return check(
        actor.expired() &&
            actor.counter() == frame_count &&
            actor.position().x == 100 &&
            actor.position().y == 200,
        "The source actor did not expire after its complete "
        "one-pass animation.");
}

bool testForwardMovementAndInterpolation() {
    osf::EnemyEffectController controller;
    controller.initialize(
        controllerRequest(10001, 0));
    const auto controller_update =
        controller.update({
            {true, {100, 200}}, nullptr, {}, {}});

    osf::EffectVisualResource visual;
    if (!loadVisual(10000010, visual)) {
        return false;
    }
    osf::RuntimeEffectActor actor;
    if (!check(
            actor.initialize(
                controller_update.actor_spawns[1],
                visual),
            "The type-one forward actor did not initialize.")) {
        return false;
    }

    const osf::GroundMap ground;
    const osf::ObjectMap objects;
    const auto first = actor.update(ground, objects);
    if (!check(
            first.intended_position.x == 280 &&
                actor.position().x == 280 &&
                actor.movementCounter() == 1 &&
                first.target_collision_active &&
                !first.environment_collision,
            "A forward actor moved on update zero or omitted its "
            "inclusive collision window.")) {
        return false;
    }

    const auto second = actor.update(ground, objects);
    const osf::WorldPosition halfway =
        actor.renderPosition(0.5);
    return check(
        second.intended_position.x == 340 &&
            second.intended_position.y == 200 &&
            actor.previousPosition().x == 280 &&
            actor.position().x == 340 &&
            halfway.x == 310 &&
            halfway.y == 200 &&
            actor.counter() == 2 &&
            !actor.expired(),
        "The forward actor did not use its start point, speed, "
        "counter, or interpolated snapshots.");
}

bool testEnvironmentCollisionAndExpiry() {
    osf::EnemyEffectController controller;
    controller.initialize(
        controllerRequest(10001, 0));
    auto controller_update =
        controller.update({
            {true, {-180, 0}}, nullptr, {}, {}});
    osf::RuntimeEffectActorSpawnRequest request =
        controller_update.actor_spawns[1];
    request.environment_audio = {3, 77};

    osf::EffectVisualResource visual;
    if (!loadVisual(10000010, visual)) {
        return false;
    }
    osf::RuntimeEffectActor actor;
    actor.initialize(request, visual);
    const osf::GroundMap ground;
    const osf::ObjectMap objects =
        oneBlockingObject(100, 0);
    const auto first = actor.update(ground, objects);
    const auto second = actor.update(ground, objects);
    if (!check(
            !first.environment_collision &&
                second.intended_position.x == 60 &&
                second.environment_collision &&
                second.expired &&
                second.audio.size() == 1 &&
                second.audio[0].sound.bank == 3 &&
                second.audio[0].sound.sample == 77 &&
                second.audio[0].position.x == 0 &&
                second.audio[0].position.y == 0 &&
                actor.position().x == 49,
            "The forward actor did not stop at the last free "
            "point and expire on static contact.")) {
        return false;
    }

    request.expire_on_environment_collision = false;
    actor.initialize(request, visual);
    actor.update(ground, objects);
    const auto retained = actor.update(ground, objects);
    return check(
        retained.environment_collision &&
            !retained.expired &&
            !actor.expired() &&
            actor.position().x == 49,
        "A non-expiring runtime actor was removed by an "
        "environment collision.");
}

bool testInclusiveTargetWindow() {
    osf::EffectVisualResource visual;
    if (!loadVisual(10000010, visual)) {
        return false;
    }
    osf::RuntimeEffectActorSpawnRequest request;
    request.resource_id = 10000010;
    request.collide_with_environment = false;
    request.target_collision_start = 2;
    request.target_collision_end = 3;
    request.animation_direction = 1;

    osf::RuntimeEffectActor actor;
    actor.initialize(request, visual);
    const osf::GroundMap ground;
    const osf::ObjectMap objects;
    for (std::int32_t update_number = 0;
         update_number < 5;
         ++update_number) {
        const auto update =
            actor.update(ground, objects);
        const bool expected =
            update_number == 2 || update_number == 3;
        if (!check(
                update.target_collision_active == expected,
                "The target collision window was not inclusive "
                "at both authored endpoints.")) {
            return false;
        }
    }
    return true;
}

bool testTargetQueryPrecedesMovement() {
    osf::EffectVisualResource visual;
    if (!loadVisual(10000010, visual)) {
        return false;
    }
    osf::RuntimeEffectActorSpawnRequest request;
    request.actor_identifier = 50000017;
    request.resource_id = 10000010;
    request.collide_with_environment = false;
    request.travel_speed = 10;
    request.target_mask = 1;
    request.target_collision_start = 1;
    request.target_collision_end = 1;
    request.animation_direction = 1;
    request.has_packet = true;
    request.packet.write(1, 3);
    request.packet.write(36, 1000);

    osf::RuntimeEffectTargetSnapshot target;
    target.kind =
        osf::RuntimeEffectTargetKind::player;
    target.character_number = 0;
    target.identifier = 701;
    target.current_life = 100;

    osf::RuntimeEffectActor actor;
    if (!check(
            actor.initialize(request, visual),
            "The target-query runtime actor did not "
            "initialize.")) {
        return false;
    }
    const osf::GroundMap ground;
    const osf::ObjectMap objects;
    osf::RetailRandom random(1);
    const std::vector<osf::RuntimeEffectTargetSnapshot>
        targets{target};
    const auto first =
        actor.update(ground, objects, targets, random);
    const auto second =
        actor.update(ground, objects, targets, random);
    const auto third =
        actor.update(ground, objects, targets, random);
    if (!check(
            !first.target_collision_active &&
                first.target_contacts.empty(),
            "The runtime actor opened its target window before "
            "the authored start update.")) {
        return false;
    }
    if (!check(
            second.target_collision_active &&
                second.target_contacts.size() == 1,
            "The runtime actor did not query the target at its "
            "pre-movement position.")) {
        return false;
    }
    return check(
        second.target_contacts[0].identifier == 701 &&
            second.target_contacts[0].impact_origin.x == 0 &&
            second.target_contacts[0].impact_origin.y == 0 &&
            second.target_contacts[0].receiver_action ==
                osf::RuntimeEffectReceiverAction::apply_packet &&
            actor.hasPacket() &&
            actor.packet()[36] == 1000 &&
            actor.position().x == 20 &&
            !third.target_collision_active &&
            third.target_contacts.empty(),
        "The runtime actor changed receiver dispatch, movement, "
        "or the inclusive target-window end.");
}

bool testObjectAndBlockedStartAudioOrder() {
    osf::EffectVisualResource visual;
    if (!loadVisual(10000010, visual)) {
        return false;
    }
    osf::RuntimeEffectActorSpawnRequest request;
    request.actor_identifier = 50000017;
    request.resource_id = 10000010;
    request.position = {100, 0};
    request.target_mask = 0x10;
    request.target_collision_start = 0;
    request.expire_on_environment_collision = true;
    request.environment_audio = {3, 77};
    request.animation_direction = 1;

    osf::RuntimeEffectTargetSnapshot target;
    target.kind =
        osf::RuntimeEffectTargetKind::scenario_object;
    target.character_number = 17000001;
    target.identifier = 801;
    target.position = {100, 0};

    osf::RuntimeEffectActor actor;
    if (!check(
            actor.initialize(request, visual),
            "The blocked-start runtime actor did not "
            "initialize.")) {
        return false;
    }
    const osf::GroundMap ground;
    const osf::ObjectMap objects =
        oneBlockingObject(100, 0);
    osf::RetailRandom random(1);
    const auto update = actor.update(
        ground,
        objects,
        {target},
        random);
    return check(
        update.target_contacts.size() == 1 &&
            update.environment_collision &&
            update.expired &&
            update.audio.size() == 2 &&
            update.audio[0].sound.bank == 3 &&
            update.audio[0].sound.sample == 77 &&
            update.audio[1].sound.bank == 3 &&
            update.audio[1].sound.sample == 77 &&
            update.audio[0].position.x == 100 &&
            update.audio[1].position.x == 100,
        "Object contact followed by a blocked static start did "
        "not preserve the retail two-callback audio order.");
}

bool testSpecialEnvironmentFiltering() {
    const osf::GroundMap ground =
        oneSpecialBlockingGround();
    const osf::ObjectMap objects;
    const osf::ObjectBounds point;
    const osf::LinearMovementStep blocked =
        osf::advanceLinearMovement(
            ground,
            objects,
            point,
            {-1, 0},
            {0, 0},
            false);
    const osf::LinearMovementStep excluded =
        osf::advanceLinearMovement(
            ground,
            objects,
            point,
            {-1, 0},
            {0, 0},
            true);
    return check(
        blocked.collided &&
            blocked.position.x == -1 &&
            !excluded.collided &&
            excluded.position.x == 0 &&
            !osf::positionIsWalkable(
                ground,
                objects,
                {0, 0},
                point,
                false) &&
            osf::positionIsWalkable(
                ground,
                objects,
                {0, 0},
                point,
                true),
        "The runtime-effect collision query did not preserve "
        "the retail special-ground exclusion flag.");
}

bool testSeparateLifetimeChartAndDisplayStatus() {
    osf::EffectVisualResource visual;
    if (!loadVisual(10000000, visual)) {
        return false;
    }
    osf::RuntimeEffectActorSpawnRequest request;
    request.resource_id = 10000000;
    request.lifetime_from_animation = true;
    request.lifetime_animation_chart = 1;
    request.animation_chart = 0;
    request.animation_direction = 8;
    request.additional_display_status = 0x80;

    osf::RuntimeEffectActor actor;
    return check(
        actor.initialize(request, visual) &&
            actor.animationChart() == 0 &&
            actor.lifetime() ==
                visual.animation()
                    .charts()[1]
                    .directions[8]
                    .frame_count &&
            actor.additionalDisplayStatus() == 0x80,
        "A runtime actor could not draw one chart while using "
        "another chart's retail lifetime and display status.");
}

bool testInvisibleDamageActor() {
    osf::RuntimeEffectActorSpawnRequest request;
    request.resource_id = -1;
    request.owner_kind = 4;
    request.source_character_number = 14000042;
    request.target_mask = 1;
    request.target_identifier = 3;
    request.position = {100, 200};
    request.judgement = {-150, -150, 150, 150};
    request.lifetime = 1;
    request.target_collision_start = 0;
    request.target_collision_end = 0;
    request.process_every_target = true;
    request.visible = false;
    request.has_packet = true;
    request.packet.write(1, 3);
    request.packet.write(36, 1000);

    osf::RuntimeEffectTargetSnapshot target;
    target.kind =
        osf::RuntimeEffectTargetKind::player;
    target.character_number = 0;
    target.identifier = 0;
    target.position = {100, 200};
    target.current_life = 100;

    osf::RuntimeEffectActor actor;
    if (!check(
            actor.initialize(
                request,
                static_cast<
                    const osf::EffectVisualResource*>(
                    nullptr)),
            "An invisible descriptor without a visual resource "
            "was rejected.")) {
        return false;
    }
    const osf::GroundMap ground;
    const osf::ObjectMap objects;
    osf::RetailRandom random(1);
    const auto update = actor.update(
        ground, objects, {target}, random);
    return check(
        !actor.visible() &&
            actor.patterns().patterns().empty() &&
            actor.animation().charts().empty() &&
            update.target_collision_active &&
            update.target_contacts.size() == 1 &&
            update.target_contacts[0].identifier == 0 &&
            update.target_contacts[0].receiver_action ==
                osf::RuntimeEffectReceiverAction::apply_packet &&
            update.expired &&
            actor.expired(),
        "The invisible one-update actor did not dispatch its "
        "packet without entering the renderer.");
}

}  // namespace

int main() {
    if (!testSourceAnimationLifetime() ||
        !testForwardMovementAndInterpolation() ||
        !testEnvironmentCollisionAndExpiry() ||
        !testInclusiveTargetWindow() ||
        !testTargetQueryPrecedesMovement() ||
        !testObjectAndBlockedStartAudioOrder() ||
        !testSpecialEnvironmentFiltering() ||
        !testSeparateLifetimeChartAndDisplayStatus() ||
        !testInvisibleDamageActor()) {
        return 1;
    }
    return 0;
}
