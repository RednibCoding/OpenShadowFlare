#include "world/scenario_data.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
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

void writeString(
    std::vector<std::uint8_t>& bytes,
    std::size_t offset,
    const std::string& value) {
    std::copy(value.begin(), value.end(), bytes.begin() + offset);
}

std::vector<std::uint8_t> scenarioFixture() {
    std::vector<std::uint8_t> bytes(0x324, 0);
    const char header[] = "MCED DATA v0000\x1a";
    std::copy(header, header + 16, bytes.begin());
    writeString(bytes, 0x10, "System\\Game\\Parameter\\Control.aid");
    writeString(bytes, 0x114, "Map\\test_01.map");
    writeI32(bytes, 0x220, 6);
    writeString(bytes, 0x224, "Test Place");

    // The scenario's entity section is intentionally opaque to this first
    // loader slice.
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
    return check(
        scenario.controllerPath() ==
                "System\\Game\\Parameter\\Control.aid" &&
            scenario.mapPath() == "Map\\test_01.map" &&
            scenario.title() == "Test Place" &&
            scenario.musicTrack() == 6 &&
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
    if (!check(
            scenario.mapPath() == "Map\\f00_01.map" &&
                scenario.title() == "Remote Town" &&
                scenario.musicTrack() == 0 &&
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
    return check(
        world.scenario().title() == "Remote Town" &&
            world.playerWorldX() == 89898 &&
            world.playerWorldY() == 2811 &&
            world.playerDirection() == 3 &&
            world.musicTrack() == 0,
        "WorldScene did not use the decoded MCT entry and music.");
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
