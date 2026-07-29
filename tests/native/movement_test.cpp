#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "world/movement_controller.hpp"
#include "world/player_actor.hpp"

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

osf::GroundMap oneBlockingGround() {
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
        appendI16(bytes, index == 25 ? 1 : 0);
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

    osf::PlayerActor player;
    player.reset({0, 0}, 1, 5);
    player.moveTo({100, 0});
    player.update(ground, objects);
    player.update(ground, objects);
    return check(
        player.position().x == 10 &&
            player.motion() == osf::PlayerMotion::idle,
        "Blocked movement did not stop at the last walkable point.");
}

bool testAxisSlide() {
    osf::GroundMap ground;
    osf::ObjectMap objects = oneBlockingObject(82, 0);
    osf::PlayerActor player;
    player.reset({0, 0}, 0, 0);
    player.moveTo({100, 100});
    player.update(ground, objects);
    return check(
        player.position().x == 2 &&
            player.position().y == 9 &&
            player.motion() == osf::PlayerMotion::walking,
        "Diagonal collision did not keep contact and try an axis slide.");
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
        !testAxisSlide() ||
        !testGroundJudgement() ||
        !testRemoteTownFixture()) {
        return 1;
    }
    return 0;
}
