#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "resources/character_visual_resource.hpp"
#include "world/actor_direction.hpp"
#include "world/movement_controller.hpp"
#include "world/npc_actor.hpp"
#include "world/player_actor.hpp"
#include "world/player_attack_target.hpp"
#include "world/world_pointer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
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

osf::GroundMap oneBlockingGround(bool blocked = true) {
    std::vector<std::uint8_t> bytes;
    const char header[16] = "RPGSCRN_GNDv000";
    bytes.insert(bytes.end(), header, header + sizeof(header));
    appendI32(bytes, 2);
    appendI32(bytes, 1);
    appendI32(bytes, 64);
    appendI32(bytes, 48);
    appendI32(bytes, 160);
    appendI32(bytes, 160);
    bytes.push_back(0);
    for (std::int32_t index = 0; index < 6; ++index) {
        appendI16(bytes, 0);
    }
    bytes.push_back(0);
    for (std::int32_t index = 0; index < 36; ++index) {
        appendI16(
            bytes,
            blocked && index == 25 ? 1 : 0);
    }
    osf::GroundMap result;
    result.decode(bytes);
    return result;
}

bool testProjection() {
    const osf::ScreenPosition screen =
        osf::calculateRealPosition({100, 0});
    const osf::WorldPosition world =
        osf::calculateWorldPosition(screen);
    const osf::WorldPosition negative =
        osf::calculateWorldPosition({-1, 0});
    return check(
        screen.x == 15 &&
            screen.y == 10 &&
            world.x == 100 &&
            world.y == 0 &&
            negative.x == -4 &&
            negative.y == 3,
        "RKC_RPGSCRN coordinate conversion differs from retail.");
}

bool testDirections() {
    const std::int32_t directions[] = {
        osf::retailDirectionForVector(1, 1),
        osf::retailDirectionForVector(1, 0),
        osf::retailDirectionForVector(1, -1),
        osf::retailDirectionForVector(0, -1),
        osf::retailDirectionForVector(-1, -1),
        osf::retailDirectionForVector(-1, 0),
        osf::retailDirectionForVector(-1, 1),
        osf::retailDirectionForVector(0, 1),
    };
    for (std::int32_t index = 0; index < 8; ++index) {
        if (directions[index] != index) {
            return check(
                false,
                "The retail eight-way direction map is incorrect.");
        }
    }
    constexpr double degrees_to_radians =
        0.01745328888888889;
    constexpr std::array<double, 8> expected_angles{{
        315.0 * degrees_to_radians,
        0.0,
        45.0 * degrees_to_radians,
        90.0 * degrees_to_radians,
        135.0 * degrees_to_radians,
        180.0 * degrees_to_radians,
        225.0 * degrees_to_radians,
        270.0 * degrees_to_radians,
    }};
    for (std::int32_t direction = 0;
         direction < 8;
         ++direction) {
        if (std::abs(
                osf::retailAngleForDirection(direction) -
                expected_angles[
                    static_cast<std::size_t>(
                        direction)]) >
            1.0e-12) {
            return check(
                false,
                "The retail direction-to-angle map is incorrect.");
        }
    }
    return true;
}

bool testRetailRectangleDistance() {
    const osf::ObjectBounds point{};
    return check(
        osf::distanceBetweenBounds(
            {0, 0}, point, {0, 0}, point) == 0 &&
            osf::distanceBetweenBounds(
                {0, 0}, point, {10, 0}, point) == 9 &&
            osf::distanceBetweenBounds(
                {0, 0}, point, {3, 4}, point) == 4 &&
            osf::distanceBetweenBounds(
                {0, 0},
                {-80, -80, 79, 79},
                {319, 0},
                {-80, -80, 79, 79}) == 159,
        "Executable rectangle distance differs from FUN_004143c0.");
}

bool testRenderInterpolation() {
    osf::GroundMap ground;
    osf::ObjectMap objects;
    osf::PlayerActor player;
    player.reset({0, 0}, 1, 5);
    player.moveTo({100, 0});
    player.update(ground, objects);
    return check(
        player.position().x == 20 &&
            player.renderPosition(0.0).x == 0 &&
            player.renderPosition(0.5).x == 10 &&
            player.renderPosition(1.0).x == 20,
        "The 60 Hz render snapshot does not interpolate a 30 Hz step.");
}

bool testMovementAndAnimation() {
    osf::GroundMap ground;
    osf::ObjectMap objects;
    osf::PlayerActor player;
    player.reset({0, 0}, 3, 5);
    player.moveTo({100, 0});
    for (std::int32_t frame = 0; frame < 5; ++frame) {
        player.update(ground, objects);
        if (player.animationChart() != 1 ||
            player.animationFrame() != frame) {
            return check(
                false,
                "Walking did not use retail CAF chart/frame timing.");
        }
    }
    if (!check(
            player.position().x == 100 &&
                player.position().y == 0 &&
                player.direction() == 1 &&
                player.motion() == osf::PlayerMotion::walking &&
                player.animationChart() == 1 &&
                player.walkingSpeedTier() == 5 &&
                player.walkingSpeed() == 20 &&
                player.runningSpeed() == 40,
            "A new player did not use the retail tier-five "
            "walk and run speeds.")) {
        return false;
    }
    player.update(ground, objects);
    if (!check(
            player.motion() == osf::PlayerMotion::idle &&
                player.animationChart() == 1,
            "The arrival tick does not retain the retail walk chart.")) {
        return false;
    }
    player.update(ground, objects);
    return check(
        player.animationChart() == 0 &&
            player.animationFrame() == 0,
        "The player did not return to idle after arriving.");
}

bool testWalkRunToggle() {
    osf::GroundMap ground;
    osf::ObjectMap objects;
    osf::PlayerActor player;
    player.reset({0, 0}, 1, 5);
    player.toggleMovementPace();
    player.moveTo({1000, 0});
    player.update(ground, objects);
    if (!check(
            player.movementPace() == osf::MovementPace::run &&
                player.motion() == osf::PlayerMotion::running &&
                player.animationChart() == 2 &&
                player.position().x == 40,
            "Run mode did not use retail action/chart two and speed 40.")) {
        return false;
    }
    player.update(ground, objects);
    if (!check(
            player.position().x == 80 &&
                player.animationFrame() == 1,
            "Running did not advance once per retail game update.")) {
        return false;
    }
    player.toggleMovementPace();
    player.update(ground, objects);
    return check(
        player.movementPace() == osf::MovementPace::walk &&
            player.motion() == osf::PlayerMotion::walking &&
            player.animationChart() == 1 &&
            player.animationFrame() == 0 &&
            player.position().x == 100,
        "Switching back to walking did not change chart and speed.");
}

bool testObjectJudgement() {
    osf::GroundMap ground;
    osf::ObjectMap objects = oneBlockingObject(90, 0);
    const osf::ObjectBounds player_bounds{-80, -80, 79, 79};
    if (!check(
            osf::positionIsWalkable(
                ground, objects, {10, 0}, player_bounds) &&
                !osf::positionIsWalkable(
                    ground, objects, {11, 0}, player_bounds),
            "Static OBL judgement does not use inclusive retail bounds.")) {
        return false;
    }

    osf::PlayerActor blocked_target;
    blocked_target.reset({0, 0}, 1, 5);
    blocked_target.moveTo({90, 0});
    blocked_target.update(ground, objects);
    if (!check(
            blocked_target.position().x == 10 &&
                blocked_target.position().y == 0 &&
                blocked_target.motion() ==
                    osf::PlayerMotion::walking,
            "The swept resolver did not return the last free point "
            "before an object contact.")) {
        return false;
    }

    osf::PlayerActor player;
    player.reset({0, 0}, 1, 5);
    player.moveTo({250, 0});
    std::int32_t greatest_y = 0;
    for (std::int32_t update = 0;
         update < 100 &&
         player.position().x != 250;
        ++update) {
        player.update(ground, objects);
        greatest_y =
            std::max(greatest_y, std::abs(player.position().y));
    }
    const bool followed =
        player.position().x == 250 &&
        player.position().y == 0 &&
        greatest_y > 79;
    if (!followed) {
        std::cerr
            << "Position: " << player.position().x
            << ", " << player.position().y
            << "; maximum detour: " << greatest_y << '\n';
    }
    return check(
        followed,
        "The retail movement controller did not follow the obstacle edge.");
}

bool testDiagonalContact() {
    osf::GroundMap ground;
    osf::ObjectMap objects = oneBlockingObject(82, 0);
    osf::PlayerActor player;
    player.reset({0, 0}, 0, 0);
    player.moveTo({100, 100});
    player.update(ground, objects);
    return check(
        player.position().x == 2 &&
            player.position().y == 2 &&
            player.motion() == osf::PlayerMotion::walking,
        "Diagonal collision did not stop at the retail swept contact.");
}

bool testGroundJudgement() {
    const osf::GroundMap ground = oneBlockingGround();
    const osf::ObjectMap objects;
    const osf::ObjectBounds point{};
    return check(
        osf::positionIsWalkable(
            ground, objects, {-1, 0}, point) &&
            !osf::positionIsWalkable(
                ground, objects, {0, 0}, point),
        "The decoded GND judgement plane does not block movement.");
}

bool testDynamicActorJudgement() {
    const osf::GroundMap ground = oneBlockingGround(false);
    const osf::ObjectMap objects;
    const std::vector<osf::MovementBlocker> actors{{
        7,
        {250, 0},
        {-40, -40, 39, 39},
    }};
    osf::PlayerActor player;
    player.reset({0, 0}, 1, 5);
    player.moveTo({500, 0});
    std::int32_t greatest_detour = 0;
    for (std::int32_t update = 0;
         update < 200 &&
         player.position().x != 500;
         ++update) {
        player.update(ground, objects, &actors);
        greatest_detour = std::max(
            greatest_detour,
            std::abs(player.position().y));
    }
    return check(
        player.position().x == 500 &&
            player.position().y == 0 &&
            greatest_detour >= 120,
        "The player did not route around a live actor's judgement "
        "rectangle.");
}

bool testDynamicActorSelfExclusion() {
    const osf::GroundMap ground =
        oneBlockingGround(false);
    const osf::ObjectMap objects;
    const std::vector<osf::MovementBlocker> actors{
        {
            7,
            {0, 0},
            {-40, -40, 39, 39},
        },
        {
            8,
            {250, 0},
            {-40, -40, 39, 39},
        },
    };
    osf::MovementController controller;
    const osf::MovementStepResult movement =
        controller.advance(
            ground,
            objects,
            {-40, -40, 39, 39},
            {0, 0},
            {500, 0},
            20,
            &actors,
            7);
    return check(
        movement.moved &&
            movement.position.x == 20 &&
            movement.position.y == 0,
        "A live actor could not exclude its own judgement "
        "rectangle.");
}

bool testNpcDynamicActorJudgement() {
    const osf::GroundMap ground =
        oneBlockingGround(false);
    const osf::ObjectMap objects;
    osf::ScenarioPerson person;
    person.id = 7;
    person.resource_id = 1;
    person.world_x = 0;
    person.world_y = 0;
    person.judgement_left = -10;
    person.judgement_top = -10;
    person.judgement_right = 10;
    person.judgement_bottom = 10;
    person.walk_speed = 10;
    person.walk_duration = 100;
    person.idle_duration = 0;
    person.wander_left = 100;
    person.wander_top = 0;
    person.wander_right = 100;
    person.wander_bottom = 0;
    person.wandering_enabled = true;
    person.initial_state_values = {1, 1, 1};

    osf::CharacterVisualResource visual;
    osf::NpcActor npc;
    if (!check(
            npc.initialize(person, visual),
            "The synthetic wandering NPC could not be initialized.")) {
        return false;
    }
    std::vector<osf::MovementBlocker> actors{
        {
            npc.movementBlockerId(),
            npc.position(),
            npc.judgement(),
        },
        {
            8,
            {50, 0},
            {-10, -10, 10, 10},
        },
    };
    std::int32_t greatest_detour = 0;
    for (std::int32_t update = 0;
         update < 100 &&
         npc.position().x != 100;
         ++update) {
        npc.update(ground, objects, &actors);
        actors[0].position = npc.position();
        greatest_detour = std::max(
            greatest_detour,
            std::abs(npc.position().y));
        const osf::WorldPosition position =
            npc.position();
        const osf::MovementBlocker& other =
            actors[1];
        const bool overlaps =
            position.x + npc.judgement().left <=
                other.position.x + other.bounds.right &&
            other.position.x + other.bounds.left <=
                position.x + npc.judgement().right &&
            position.y + npc.judgement().top <=
                other.position.y + other.bounds.bottom &&
            other.position.y + other.bounds.top <=
                position.y + npc.judgement().bottom;
        if (overlaps) {
            return check(
                false,
                "A wandering NPC entered another live actor's "
                "judgement rectangle.");
        }
    }
    return check(
        npc.position().x == 100 &&
            npc.position().y == 0 &&
            greatest_detour >= 20,
        "A wandering NPC did not route around a live actor.");
}

bool testScriptedNpcTurningFlag() {
    osf::ScenarioPerson person;
    person.id = 7;
    person.resource_id = 1;
    person.direction = 1;
    person.initial_state_values = {1, 1, 1};

    osf::CharacterVisualResource visual;
    osf::NpcActor npc;
    if (!check(
            npc.initialize(person, visual),
            "The non-turning NPC could not be initialized.")) {
        return false;
    }
    npc.faceToward({0, 100});
    if (!check(
            npc.direction() == 1,
            "A PEOPLE record ignored its disabled scripted-turning "
            "flag.")) {
        return false;
    }

    person.scripted_turning_enabled = true;
    if (!check(
            npc.initialize(person, visual),
            "The turning NPC could not be initialized.")) {
        return false;
    }
    npc.faceToward({0, 100});
    return check(
        npc.direction() ==
            osf::retailDirectionForVector(0, 100),
        "Native PEOPLE action 21 did not face its enabled target.");
}

bool testWorldPointerPriority() {
    const std::vector<osf::WorldPointerCandidate> candidates{
        {
            {osf::WorldPointerTargetKind::npc, 10},
            {0, {100, 100}, {-10, -10, 10, 10}, 0},
            0,
        },
        {
            {osf::WorldPointerTargetKind::ground_item, 20},
            {0, {100, 100}, {}, 0},
            3,
        },
        {
            {osf::WorldPointerTargetKind::enemy, 30},
            {0, {100, 100}, {}, 0},
            2,
        },
    };
    osf::WorldPointer pointer;
    pointer.update(30, 40, candidates);
    if (!check(
            pointer.active() &&
                pointer.screenX() == 30 &&
                pointer.screenY() == 40 &&
                pointer.target().kind ==
                    osf::WorldPointerTargetKind::enemy &&
                pointer.target().id == 30,
            "The default retail click priority did not prefer "
            "an enemy over other world targets.")) {
        return false;
    }

    osf::WorldPointerConfiguration configuration;
    configuration.click_priority[1] = 4;
    configuration.click_priority[2] = 0;
    configuration.click_priority[0] = 1;
    configuration.range = 99;
    pointer.configure(configuration);
    pointer.update(30, 40, candidates);
    if (!check(
        pointer.target().kind ==
                osf::WorldPointerTargetKind::npc &&
            pointer.target().id == 10 &&
            pointer.configuration().range == 4,
        "The configured retail click priority was not applied.")) {
        return false;
    }

    std::vector<osf::WorldPointerCandidate>
        ranged_candidates{
            {
                {osf::WorldPointerTargetKind::npc, 30},
                {0, {100, 100}, {}, 0},
                0,
                false,
                100,
            },
            {
                {osf::WorldPointerTargetKind::npc, 31},
                {0, {100, 100}, {}, 0},
                0,
                false,
                25,
            },
        };
    osf::WorldPointer ranged_pointer;
    ranged_pointer.update(30, 40, ranged_candidates);
    if (!check(
            ranged_pointer.target().id == 31,
            "The nearest range-only pointer candidate did not "
            "win its priority group.")) {
        return false;
    }
    ranged_candidates.push_back({
        {osf::WorldPointerTargetKind::npc, 32},
        {0, {100, 100}, {}, 0},
        0,
        true,
        400,
    });
    ranged_pointer.update(30, 40, ranged_candidates);
    if (!check(
            ranged_pointer.target().id == 32,
            "An exact cursor hit did not win over nearer "
            "range-only candidates.")) {
        return false;
    }

    osf::WorldPointerConfiguration range_configuration;
    if (!check(
            osf::worldPointerHalfSize(
                range_configuration) == 16,
            "The default retail click range has the wrong size.")) {
        return false;
    }
    range_configuration.range_enabled = false;
    return check(
        osf::worldPointerHalfSize(range_configuration) == 0,
        "Disabling the click range did not restore exact-tip "
        "picking.");
}

bool testPlayerAttackTargetRange() {
    const osf::ObjectBounds actor_bounds{
        -80, -80, 79, 79,
    };
    osf::PlayerAttackTargetSnapshot target{
        7,
        {319, 0},
        actor_bounds,
        10,
        true,
        true,
    };
    if (!check(
            osf::kRetailPlayerAttackRange == 0x9f &&
                osf::classifyPlayerAttackTarget(
                    {0, 0},
                    actor_bounds,
                    target) ==
                    osf::PlayerAttackTargetDisposition::ready,
            "The inclusive retail player attack-range edge was "
            "not accepted.")) {
        return false;
    }

    target.position.x = 320;
    if (!check(
            osf::classifyPlayerAttackTarget(
                {0, 0},
                actor_bounds,
                target) ==
                osf::PlayerAttackTargetDisposition::approach,
            "A target one unit beyond retail attack range did "
            "not request an approach.")) {
        return false;
    }

    osf::PlayerAttackTargetController controller;
    if (!check(
            controller.command(
                {0, 0},
                actor_bounds,
                target) ==
                    osf::PlayerAttackTargetDisposition::approach &&
                controller.approachTargetId() == 7 &&
                controller.readyTargetId() == -1,
            "The retail attack target did not enter its approach "
            "state.")) {
        return false;
    }
    target.position.x = 319;
    if (!check(
            controller.refresh(
                {0, 0},
                actor_bounds,
                &target) ==
                    osf::PlayerAttackTargetDisposition::ready &&
                controller.approachTargetId() == -1 &&
                controller.readyTargetId() == 7,
            "Reaching retail attack range did not promote the "
            "approach target to ready.")) {
        return false;
    }
    if (!check(
            controller.takeReadyTargetId() == 7 &&
                controller.readyTargetId() == -1,
            "Starting an attack did not consume its ready target.")) {
        return false;
    }
    controller.command({0, 0}, actor_bounds, target);
    target.life = 0;
    if (!check(
            !controller.validateReady(&target) &&
                controller.readyTargetId() == -1,
            "A defeated ready target was retained by the player "
            "combat controller.")) {
        return false;
    }
    target.life = 10;
    controller.command({0, 0}, actor_bounds, target);
    controller.cancel();
    if (!check(
            controller.approachTargetId() == -1 &&
                controller.readyTargetId() == -1,
            "Cancelling a player command retained its combat "
            "target.")) {
        return false;
    }

    target.position.x = 320;
    controller.command({0, 0}, actor_bounds, target);
    if (!check(
            controller.refresh(
                {0, 0},
                actor_bounds,
                nullptr) ==
                    osf::PlayerAttackTargetDisposition::rejected &&
                controller.approachTargetId() == -1 &&
                controller.readyTargetId() == -1,
            "A removed enemy did not cancel the pending player "
            "approach.")) {
        return false;
    }

    target.life = 0;
    if (!check(
            osf::classifyPlayerAttackTarget(
                {0, 0},
                actor_bounds,
                target) ==
                osf::PlayerAttackTargetDisposition::rejected,
            "A defeated enemy remained a valid player target.")) {
        return false;
    }
    target.life = 10;
    target.visible = false;
    if (!check(
            osf::classifyPlayerAttackTarget(
                {0, 0},
                actor_bounds,
                target) ==
                osf::PlayerAttackTargetDisposition::rejected,
            "A hidden enemy remained a valid player target.")) {
        return false;
    }
    target.visible = true;
    target.pointer_enabled = false;
    return check(
        osf::classifyPlayerAttackTarget(
            {0, 0},
            actor_bounds,
            target) ==
            osf::PlayerAttackTargetDisposition::rejected,
        "A pointer-disabled enemy remained a valid player target.");
}

bool testRemoteTownFixture() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "Map";
    const std::filesystem::path ground_path =
        root / "Ground" / "f00_01.Gnd";
    if (!std::filesystem::is_regular_file(ground_path)) {
        return true;
    }

    osf::GroundMap ground;
    osf::ObjectMap objects;
    std::string error;
    if (!check(
            ground.load(ground_path, &error),
            "The retail Remote Town GND no longer decodes.")) {
        std::cerr << error << '\n';
        return false;
    }
    if (!check(
            objects.load(
                root / "Object" / "f00_01.Obl",
                &error),
            "The retail Remote Town OBL no longer decodes.")) {
        std::cerr << error << '\n';
        return false;
    }
    return check(
        ground.width() == 300 &&
            ground.height() == 300 &&
            ground.judgeWidth() == 852 &&
            ground.judgeHeight() == 852 &&
            ground.judgeOffsetX() == -1 &&
            ground.judgeOffsetY() == -401 &&
            objects.objects().size() == 279 &&
            osf::positionIsWalkable(
                ground,
                objects,
                {89898, 2811},
                {-80, -80, 79, 79}),
        "Remote Town judgement metadata or spawn collision differs.");
#else
    return true;
#endif
}

}  // namespace

int main() {
    if (!testProjection() ||
        !testDirections() ||
        !testRetailRectangleDistance() ||
        !testRenderInterpolation() ||
        !testMovementAndAnimation() ||
        !testWalkRunToggle() ||
        !testObjectJudgement() ||
        !testDiagonalContact() ||
        !testGroundJudgement() ||
        !testDynamicActorJudgement() ||
        !testDynamicActorSelfExclusion() ||
        !testNpcDynamicActorJudgement() ||
        !testScriptedNpcTurningFlag() ||
        !testWorldPointerPriority() ||
        !testPlayerAttackTargetRange() ||
        !testRemoteTownFixture()) {
        return 1;
    }
    return 0;
}
