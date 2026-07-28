#include "gapi/gapi.hpp"
#include "render/gameplay_renderer.hpp"
#include "world/scenario_data.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

struct NpcPatternCall {
    bool shadow = false;
    std::size_t pattern = 0;
    osf::gapi::PatternDraw draw;
};

class NpcRecordingBackend final : public osf::gapi::Backend {
public:
    const osf::gapi::NjpImage* patterns = nullptr;
    const osf::gapi::NjpImage* shadows = nullptr;
    std::vector<NpcPatternCall> calls;

    void beginFrame(osf::gapi::Color) override {}

    bool drawPattern(
        const osf::gapi::NjpImage& image,
        std::size_t pattern,
        const osf::gapi::PatternDraw& draw) override {
        if (&image == patterns) {
            calls.push_back({false, pattern, draw});
        } else if (&image == shadows) {
            calls.push_back({true, pattern, draw});
        }
        return true;
    }

    bool drawBitmap(
        const osf::gapi::BitmapImage&,
        const osf::gapi::BitmapDraw&) override {
        return true;
    }

    bool drawText(
        const osf::gapi::NjpImage&,
        std::string_view,
        const osf::gapi::TextDraw&) override {
        return true;
    }

    bool drawRectangle(
        const osf::gapi::RectangleDraw&) override {
        return true;
    }

    void endFrame() override {}
};

void writeI32(
    std::vector<std::uint8_t>& bytes,
    std::size_t offset,
    std::int32_t value) {
    const std::uint32_t raw =
        static_cast<std::uint32_t>(value);
    bytes[offset] = static_cast<std::uint8_t>(raw);
    bytes[offset + 1] =
        static_cast<std::uint8_t>(raw >> 8u);
    bytes[offset + 2] =
        static_cast<std::uint8_t>(raw >> 16u);
    bytes[offset + 3] =
        static_cast<std::uint8_t>(raw >> 24u);
}

void appendI32(
    std::vector<std::uint8_t>& bytes,
    std::int32_t value) {
    const std::size_t offset = bytes.size();
    bytes.resize(offset + 4);
    writeI32(bytes, offset, value);
}

void appendI16(
    std::vector<std::uint8_t>& bytes,
    std::int16_t value) {
    const std::uint16_t raw =
        static_cast<std::uint16_t>(value);
    bytes.push_back(static_cast<std::uint8_t>(raw));
    bytes.push_back(
        static_cast<std::uint8_t>(raw >> 8u));
}

void writeString(
    std::vector<std::uint8_t>& bytes,
    std::size_t offset,
    const std::string& value) {
    std::copy(value.begin(), value.end(), bytes.begin() + offset);
}

void appendCommonEntity(
    std::vector<std::uint8_t>& bytes,
    std::int32_t id,
    std::int32_t resource_id,
    const std::string& name,
    std::int32_t world_x,
    std::int32_t world_y,
    std::int32_t direction,
    bool custom_parts) {
    appendI32(bytes, id);
    appendI32(bytes, resource_id);
    appendI32(
        bytes, static_cast<std::int32_t>(name.size()));
    bytes.insert(bytes.end(), name.begin(), name.end());
    if (!name.empty()) {
        appendI32(bytes, 0x00e0e0e0);
    }
    appendI32(bytes, 80);
    appendI32(bytes, world_x);
    appendI32(bytes, world_y);
    appendI32(bytes, -80);
    appendI32(bytes, -80);
    appendI32(bytes, 79);
    appendI32(bytes, 79);
    appendI32(bytes, direction);
    appendI32(bytes, 3);
    appendI32(bytes, 1);
    appendI32(bytes, 0);
    appendI32(bytes, 1);
    appendI32(bytes, custom_parts ? 1 : 0);
    if (custom_parts) {
        appendI32(bytes, 7);
        const std::int32_t visibility[] = {
            1, 1, 1, 1, 0, 0, 1,
        };
        for (std::int32_t value : visibility) {
            appendI32(bytes, value);
        }
        for (std::int32_t channel = 0;
             channel < 3;
             ++channel) {
            for (std::int32_t part = 0; part < 7; ++part) {
                appendI16(bytes, 1000);
            }
        }
    }
    appendI32(bytes, 1);
}

std::vector<std::uint8_t> scenarioFixture() {
    std::vector<std::uint8_t> bytes(0x324, 0);
    const char header[] = "MCED DATA v0000\x1a";
    std::copy(header, header + 16, bytes.begin());
    writeString(bytes, 0x10, "System\\Game\\Parameter\\Control.aid");
    writeString(bytes, 0x114, "Map\\test_01.map");
    writeI32(bytes, 0x220, 6);
    writeString(bytes, 0x224, "Test Place");

    appendI32(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 0);

    appendI32(bytes, 1);
    appendCommonEntity(
        bytes, 50, -1, "", 10, 20, 0, false);
    bytes.insert(bytes.end(), 0x34, 0);

    appendI32(bytes, 2);
    appendCommonEntity(
        bytes, 4, 13, "Test NPC", 300, 400, 7, true);
    for (std::int32_t value :
         {10, 30, 30, 0, -40, -20, 60, 80, 1, 0, -65}) {
        appendI32(bytes, value);
    }
    appendCommonEntity(
        bytes, 5, 8, "Other", 500, 600, 1, false);
    bytes.insert(bytes.end(), 0x2c, 0);

    // Later entity groups remain outside this focused decoder slice.
    bytes.insert(bytes.end(), 23, 0xcc);

    appendI32(bytes, 2);
    appendI32(bytes, 0);
    appendI32(bytes, 100);
    appendI32(bytes, 200);
    appendI32(bytes, 3);
    appendI32(bytes, 7);
    appendI32(bytes, -50);
    appendI32(bytes, 60);
    appendI32(bytes, 7);

    appendI32(bytes, 11);
    appendI32(bytes, 22);
    appendI32(bytes, 33);
    return bytes;
}

bool testFixture() {
    osf::ScenarioData scenario;
    std::string error;
    if (!check(
            scenario.decode(scenarioFixture(), &error),
            "A valid synthetic scenario was rejected.")) {
        std::cerr << error << '\n';
        return false;
    }

    const osf::ScenarioEntry* first = scenario.findEntry(0);
    const osf::ScenarioEntry* second = scenario.findEntry(7);
    const osf::ScenarioPerson* person =
        scenario.people().empty()
            ? nullptr
            : &scenario.people().front();
    return check(
        scenario.controllerPath() ==
                "System\\Game\\Parameter\\Control.aid" &&
            scenario.mapPath() == "Map\\test_01.map" &&
            scenario.title() == "Test Place" &&
            scenario.musicTrack() == 6 &&
            scenario.people().size() == 2 &&
            person &&
            person->id == 4 &&
            person->resource_id == 13 &&
            person->name == "Test NPC" &&
            person->name_color == 0x00e0e0e0 &&
            person->world_x == 300 &&
            person->world_y == 400 &&
            person->direction == 7 &&
            person->part_visibility ==
                std::vector<std::int32_t>{
                    1, 1, 1, 1, 0, 0, 1} &&
            person->red_strength ==
                std::vector<std::int16_t>(
                    7, static_cast<std::int16_t>(1000)) &&
            person->walk_speed == 10 &&
            person->walk_duration == 30 &&
            person->idle_duration == 30 &&
            person->wander_bounds_relative &&
            person->wander_left == -40 &&
            person->wander_top == -20 &&
            person->wander_right == 60 &&
            person->wander_bottom == 80 &&
            person->wandering_enabled &&
            scenario.entries().size() == 2 &&
            first &&
            first->world_x == 100 &&
            first->world_y == 200 &&
            first->direction == 3 &&
            second &&
            second->world_x == -50 &&
            second->world_y == 60 &&
            second->direction == 7 &&
            scenario.findEntry(99) == nullptr,
        "The synthetic scenario fields were decoded incorrectly.");
}

bool testMalformedData() {
    osf::ScenarioData scenario;
    std::vector<std::uint8_t> bytes = scenarioFixture();
    bytes[0] = 'X';
    if (!check(
            !scenario.decode(bytes),
            "A scenario with the wrong MCED signature was accepted.")) {
        return false;
    }

    bytes = scenarioFixture();
    bytes.resize(0x324);
    return check(
        !scenario.decode(bytes),
        "A scenario without an entry table was accepted.");
}

bool testRetailRemoteTown() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    const std::filesystem::path path =
        data_root / "Scenario" / "00000000" / "Scenario.Mct";
    if (!std::filesystem::is_regular_file(path)) {
        return true;
    }

    osf::ScenarioData scenario;
    std::string error;
    if (!check(
            scenario.load(path, &error),
            "The retail Remote Town scenario no longer decodes.")) {
        std::cerr << error << '\n';
        return false;
    }
    const osf::ScenarioEntry* entry = scenario.findEntry(0);
    const osf::ScenarioPerson* ostare =
        scenario.people().empty()
            ? nullptr
            : &scenario.people().front();
    if (!check(
            scenario.mapPath() == "Map\\f00_01.map" &&
                scenario.title() == "Remote Town" &&
                scenario.musicTrack() == 0 &&
                scenario.people().size() == 7 &&
                ostare &&
                ostare->id == 0 &&
                ostare->resource_id == 13 &&
                ostare->name == "Ostare" &&
                ostare->name_color == 0x00e0e0e0 &&
                ostare->world_x == 91467 &&
                ostare->world_y == 1532 &&
                ostare->direction == 7 &&
                ostare->part_visibility.size() == 256 &&
                ostare->part_visibility[0] == 1 &&
                ostare->part_visibility[4] == 0 &&
                ostare->part_visibility[5] == 0 &&
                ostare->part_visibility[6] == 1 &&
                ostare->walk_speed == 10 &&
                ostare->walk_duration == 30 &&
                ostare->idle_duration == 30 &&
                ostare->wander_bounds_relative &&
                ostare->wander_left == -437 &&
                ostare->wander_top == -223 &&
                ostare->wander_right == 269 &&
                ostare->wander_bottom == 231 &&
                ostare->wandering_enabled &&
                scenario.entries().size() == 12 &&
                entry &&
                entry->world_x == 89898 &&
                entry->world_y == 2811 &&
                entry->direction == 3,
            "The retail Remote Town scenario differs from the traced data.")) {
        return false;
    }

    osf::WorldScene world;
    if (!check(
            world.loadInitialScenario(data_root, 0, &error),
            "Remote Town could not be loaded through Scenario.Mct.")) {
        std::cerr << error << '\n';
        return false;
    }
    if (!check(
        world.scenario().title() == "Remote Town" &&
            world.playerWorldX() == 89898 &&
            world.playerWorldY() == 2811 &&
            world.playerDirection() == 3 &&
            world.musicTrack() == 0 &&
            world.npcs().size() == 1 &&
            world.npcs()[0].id() == 0 &&
            world.npcs()[0].resourceId() == 13 &&
            world.npcs()[0].name() == "Ostare" &&
            world.npcs()[0].position().x == 91467 &&
            world.npcs()[0].position().y == 1532 &&
            world.npcs()[0].direction() == 7 &&
            world.npcs()[0].animationChart() == 0 &&
            world.npcs()[0].animationFrame() == 0 &&
            world.npcs()[0].partEnabled(3) &&
            !world.npcs()[0].partEnabled(4) &&
            !world.npcs()[0].partEnabled(5) &&
            world.npcs()[0].partEnabled(6),
        "WorldScene did not build Ostare from the decoded MCT record.")) {
        return false;
    }

    NpcRecordingBackend renderer;
    renderer.patterns = &world.npcs()[0].patterns();
    renderer.shadows = &world.npcs()[0].shadowPatterns();
    osf::renderWorld(renderer, world, 500);
    if (!check(
            renderer.calls.size() == 3 &&
                renderer.calls[0].shadow &&
                renderer.calls[0].pattern == 280 &&
                renderer.calls[0].draw.x == 747 &&
                renderer.calls[0].draw.y == 269 &&
                renderer.calls[0].draw.opacity == 500 &&
                !renderer.calls[1].shadow &&
                renderer.calls[1].pattern == 1744 &&
                !renderer.calls[2].shadow &&
                renderer.calls[2].pattern == 1784,
            "Ostare's idle frame, part mask, shadow, or placement differs.")) {
        return false;
    }

    world.update();
    if (!check(
            world.npcs()[0].animationFrame() == 0,
            "Ostare skipped the first retail idle frame.")) {
        return false;
    }
    world.update();
    if (!check(
            world.npcs()[0].animationFrame() == 1,
            "Ostare's idle animation does not advance at game-update cadence.")) {
        return false;
    }

    for (std::int32_t update = 2; update < 30; ++update) {
        world.update();
    }
    if (!check(
            world.npcs()[0].position().x == 91467 &&
                world.npcs()[0].position().y == 1532 &&
                world.npcs()[0].animationChart() == 0,
            "Ostare did not keep the retail 30-update idle pause.")) {
        return false;
    }
    world.update();
    const osf::WorldPosition walking_position =
        world.npcs()[0].position();
    return check(
        world.npcs()[0].animationChart() == 1 &&
            (walking_position.x != 91467 ||
             walking_position.y != 1532) &&
            walking_position.x >= 91030 &&
            walking_position.x <= 91736 &&
            walking_position.y >= 1309 &&
            walking_position.y <= 1763,
        "Ostare did not begin the retail bounded wander action.");
#else
    return true;
#endif
}

}  // namespace

int main() {
    return testFixture() &&
                   testMalformedData() &&
                   testRetailRemoteTown()
               ? 0
               : 1;
}
