#include "gapi/gapi.hpp"
#include "render/gameplay_renderer.hpp"
#include "world/ground_item.hpp"
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

bool testGroundItemCreation() {
    osf::RetailRandom random;
    std::vector<osf::GroundItem> items;
    if (!check(
            osf::createGroundItems(
                items, random, 2, 45, {100, 200}, -1, -1) &&
                items.size() == 1 &&
                items[0].category == 2 &&
                items[0].definition_id == 45 &&
                items[0].quantity == 1 &&
                items[0].position.x == 100 &&
                items[0].position.y == 200,
            "Ordinary script items were not created at their exact point.")) {
        return false;
    }
    if (!check(
            osf::createGroundItems(
                items,
                random,
                4,
                0,
                {100, 200},
                25000,
                25000) &&
                items.size() == 4 &&
                items[1].quantity == 10000 &&
                items[2].quantity == 10000 &&
                items[3].quantity == 5000 &&
                items[1].position.x == 300 &&
                items[1].position.y == 200,
            "Retail money splitting or radial placement differs.")) {
        return false;
    }
    osf::GroundItem bouncing = items.front();
    osf::updateGroundItem(bouncing);
    if (!check(
            bouncing.height == 160 &&
                bouncing.vertical_velocity == 1320 &&
                bouncing.bounce_state == 0,
            "A new ground item did not begin its retail drop arc.")) {
        return false;
    }
    for (std::int32_t update = 1; update < 19; ++update) {
        osf::updateGroundItem(bouncing);
    }
    if (!check(
            bouncing.height == 0 &&
                bouncing.bounce_state == 2,
            "A ground item did not settle after its two retail bounces.")) {
        return false;
    }
    return check(
        !osf::createGroundItems(
            items, random, 4, 0, {}, 10, 9) &&
            items.size() == 4,
        "An invalid script money range created a ground item.");
}

struct NpcPatternCall {
    bool shadow = false;
    std::size_t pattern = 0;
    osf::gapi::PatternDraw draw;
};

struct TextCall {
    std::string text;
    osf::gapi::TextDraw draw;
};

class NpcRecordingBackend final : public osf::gapi::Backend {
public:
    const osf::gapi::NjpImage* patterns = nullptr;
    const osf::gapi::NjpImage* shadows = nullptr;
    const osf::gapi::NjpImage* speech = nullptr;
    const osf::gapi::NjpImage* item_patterns = nullptr;
    const osf::gapi::NjpImage* item_shadows = nullptr;
    std::vector<NpcPatternCall> calls;
    std::vector<NpcPatternCall> speech_calls;
    std::vector<NpcPatternCall> item_calls;
    std::vector<TextCall> text_calls;
    std::vector<osf::gapi::RectangleDraw> rectangles;

    void beginFrame(osf::gapi::Color) override {}

    bool drawPattern(
        const osf::gapi::NjpImage& image,
        std::size_t pattern,
        const osf::gapi::PatternDraw& draw) override {
        if (&image == patterns) {
            calls.push_back({false, pattern, draw});
        } else if (&image == shadows) {
            calls.push_back({true, pattern, draw});
        } else if (&image == speech) {
            speech_calls.push_back({false, pattern, draw});
        } else if (&image == item_patterns) {
            item_calls.push_back({false, pattern, draw});
        } else if (&image == item_shadows) {
            item_calls.push_back({true, pattern, draw});
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
        std::string_view text,
        const osf::gapi::TextDraw& draw) override {
        text_calls.push_back({std::string(text), draw});
        return true;
    }

    bool drawRectangle(
        const osf::gapi::RectangleDraw& draw) override {
        rectangles.push_back(draw);
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
            person->label_height == 80 &&
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
                ostare->label_height == 80 &&
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
            world.npcs()[0].nameColor() == 0x00e0e0e0 &&
            world.npcs()[0].labelHeight() == 80 &&
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
                renderer.calls[1].draw.red_strength == 1000 &&
                !renderer.calls[2].shadow &&
                renderer.calls[2].pattern == 1784,
            "Ostare's idle frame, part mask, shadow, or placement differs.")) {
        return false;
    }

    osf::gapi::NjpImage font;
    if (!check(
            font.load(
                data_root / "System" / "Common" / "Pattern" /
                    "Font01.njp",
                &error),
            "The retail gameplay font could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }
    renderer.speech = &world.speechPatterns();
    renderer.calls.clear();
    world.updatePointerHover(747, 269);
    osf::renderWorld(renderer, world, 500, &font);
    if (!check(
            world.hoveredNpcId() == 0 &&
                renderer.calls.size() == 3 &&
                renderer.calls[1].draw.red_strength == 1300 &&
                renderer.calls[1].draw.green_strength == 1300 &&
                renderer.calls[1].draw.blue_strength == 1300 &&
                renderer.rectangles.size() == 1 &&
                renderer.rectangles[0].x == 725 &&
                renderer.rectangles[0].y == 187 &&
                renderer.rectangles[0].width == 41 &&
                renderer.rectangles[0].height == 15 &&
                renderer.rectangles[0].opacity == 500 &&
                renderer.text_calls.size() == 2 &&
                renderer.text_calls[0].text == "Ostare" &&
                renderer.text_calls[0].draw.x == 730 &&
                renderer.text_calls[0].draw.y == 190 &&
                renderer.text_calls[1].draw.x == 729 &&
                renderer.text_calls[1].draw.y == 189 &&
                renderer.text_calls[1].draw.color.red == 224,
            "Ostare's retail hover tint or nameplate differs.")) {
        return false;
    }

    renderer.calls.clear();
    renderer.speech_calls.clear();
    renderer.text_calls.clear();
    renderer.rectangles.clear();
    if (!check(
            world.scenarioScript().messages().size() == 61 &&
                world.commandWorldInteraction(747, 269) &&
                world.conversationActive() &&
                world.conversationMessageId() == 1000000 &&
                world.conversationText().rfind(
                    "Thank you for coming. I am Ostare", 0) == 0,
            "Ostare's click did not enter the retail status-zero script.")) {
        return false;
    }
    osf::renderWorld(renderer, world, 500, &font);
    if (!check(
            world.hoveredNpcId() == -1 &&
                world.conversationActorId() == 0 &&
                renderer.speech_calls.size() == 5 &&
                renderer.speech_calls[0].pattern == 0 &&
                renderer.speech_calls[0].draw.x == 566 &&
                renderer.speech_calls[0].draw.y == 96 &&
                renderer.speech_calls[1].pattern == 2 &&
                renderer.speech_calls[1].draw.x == 943 &&
                renderer.speech_calls[1].draw.y == 96 &&
                renderer.speech_calls[2].pattern == 1 &&
                renderer.speech_calls[2].draw.x == 566 &&
                renderer.speech_calls[2].draw.y == 173 &&
                renderer.speech_calls[3].pattern == 3 &&
                renderer.speech_calls[3].draw.x == 943 &&
                renderer.speech_calls[3].draw.y == 173 &&
                renderer.speech_calls[4].pattern == 4 &&
                renderer.speech_calls[4].draw.x == 754 &&
                renderer.speech_calls[4].draw.y == 178 &&
                renderer.rectangles.size() == 13 &&
                renderer.rectangles[0].x == 570 &&
                renderer.rectangles[0].y == 100 &&
                renderer.rectangles[0].width == 378 &&
                renderer.rectangles[0].height == 78 &&
                renderer.rectangles[0].color.red == 255 &&
                renderer.rectangles[1].x == 575 &&
                renderer.rectangles[1].y == 96 &&
                renderer.rectangles[1].width == 368 &&
                renderer.rectangles[1].height == 2 &&
                renderer.rectangles[2].color.red == 160 &&
                renderer.rectangles[3].color.red == 224 &&
                renderer.rectangles[7].x == 566 &&
                renderer.rectangles[7].y == 105 &&
                renderer.rectangles[10].x == 950 &&
                renderer.text_calls.size() == 1 &&
                renderer.text_calls[0].draw.x == 579 &&
                renderer.text_calls[0].draw.y == 109 &&
                renderer.text_calls[0].draw.color.red == 0,
            "Ostare's first message did not use the retail actor bubble.")) {
        return false;
    }
    const osf::WorldPosition interaction_position =
        world.npcs()[0].position();
    for (std::int32_t update = 0; update < 40; ++update) {
        world.update();
    }
    if (!check(
            world.npcs()[0].position().x ==
                    interaction_position.x &&
                world.npcs()[0].position().y ==
                    interaction_position.y &&
                world.npcs()[0].animationChart() == 0,
            "Ostare kept wandering while his script message was open.")) {
        return false;
    }
    world.advanceConversation();
    if (!check(
            world.conversationActive() &&
                world.conversationActorId() == 0 &&
                world.conversationMessageId() == 1000001,
            "Ostare's first message did not enter its retail callback.")) {
        return false;
    }
    world.advanceConversation();
    if (!check(
            world.conversationActive() &&
                world.conversationMessageId() == 1000002,
            "Ostare's second opening message did not follow retail.")) {
        return false;
    }
    world.advanceConversation();
    if (!check(
            world.conversationActive() &&
                world.conversationMessageId() == 1000003 &&
                world.groundItems().size() == 4 &&
                world.groundItems()[0].category == 0 &&
                world.groundItems()[0].definition_id == 0 &&
                world.groundItems()[0].quantity == 1 &&
                world.groundItems()[0].position.x ==
                    interaction_position.x + 200 &&
                world.groundItems()[0].position.y ==
                    interaction_position.y &&
                world.groundItems()[0].resource_id == 0 &&
                world.groundItems()[0].animation_chart == 0 &&
                world.groundItems()[0].red_strength == 1000 &&
                world.groundItems()[0].green_strength == 1000 &&
                world.groundItems()[0].blue_strength == 1000 &&
                world.groundItems()[1].category == 1 &&
                world.groundItems()[1].definition_id == 1000000 &&
                world.groundItems()[1].position.x ==
                    interaction_position.x &&
                world.groundItems()[1].position.y ==
                    interaction_position.y + 200 &&
                world.groundItems()[1].resource_id == 0 &&
                world.groundItems()[1].animation_chart == 5 &&
                world.groundItems()[1].red_strength == 900 &&
                world.groundItems()[1].green_strength == 800 &&
                world.groundItems()[1].blue_strength == 500 &&
                world.groundItems()[2].category == 0 &&
                world.groundItems()[2].definition_id == 100 &&
                world.groundItems()[2].position.x ==
                    interaction_position.x + 200 &&
                world.groundItems()[2].position.y ==
                    interaction_position.y - 200 &&
                world.groundItems()[2].resource_id == 0 &&
                world.groundItems()[2].animation_chart == 36 &&
                world.groundItems()[2].red_strength == 1000 &&
                world.groundItems()[2].green_strength == 1000 &&
                world.groundItems()[2].blue_strength == 1000 &&
                world.groundItems()[3].category == 4 &&
                world.groundItems()[3].definition_id == 0 &&
                world.groundItems()[3].quantity == 200 &&
                world.groundItems()[3].position.x ==
                    interaction_position.x + 200 &&
                world.groundItems()[3].position.y ==
                    interaction_position.y &&
                world.groundItems()[3].resource_id == 0 &&
                world.groundItems()[3].animation_chart == 30 &&
                world.groundItems()[3].red_strength == 1000 &&
                world.groundItems()[3].green_strength == 1000 &&
                world.groundItems()[3].blue_strength == 1000,
            "Ostare's opening quest did not create its retail ground items.")) {
        return false;
    }
    const osf::ItemWorldResource* item_resource =
        world.itemWorldResource(0);
    renderer.item_patterns =
        item_resource ? &item_resource->patterns() : nullptr;
    renderer.item_shadows =
        item_resource
            ? &item_resource->shadowPatterns()
            : nullptr;
    renderer.item_calls.clear();
    osf::renderWorld(renderer, world, 500, &font);
    if (!check(
            item_resource &&
                item_resource->patterns().palettes().size() > 72 &&
                item_resource->shadowPatterns().palettes().size() == 1 &&
                renderer.item_calls.size() == 8 &&
                renderer.item_calls[0].shadow &&
                renderer.item_calls[0].pattern == 36 &&
                renderer.item_calls[0].draw.palette == -1 &&
                renderer.item_calls[0].draw.x == 807 &&
                renderer.item_calls[0].draw.y == 269 &&
                renderer.item_calls[1].shadow &&
                renderer.item_calls[1].pattern == 0 &&
                renderer.item_calls[1].draw.palette == -1 &&
                renderer.item_calls[2].shadow &&
                renderer.item_calls[2].pattern == 5 &&
                renderer.item_calls[2].draw.palette == -1 &&
                renderer.item_calls[3].shadow &&
                renderer.item_calls[3].pattern == 30 &&
                renderer.item_calls[3].draw.palette == -1 &&
                !renderer.item_calls[4].shadow &&
                renderer.item_calls[4].pattern == 113 &&
                renderer.item_calls[4].draw.palette == 72 &&
                renderer.item_calls[4].draw.x == 807 &&
                renderer.item_calls[4].draw.y == 269 &&
                renderer.item_calls[5].pattern == 77 &&
                renderer.item_calls[5].draw.palette == 0 &&
                renderer.item_calls[5].draw.x == 777 &&
                renderer.item_calls[5].draw.y == 289 &&
                renderer.item_calls[6].pattern == 82 &&
                renderer.item_calls[6].draw.palette == 10 &&
                renderer.item_calls[6].draw.red_strength == 900 &&
                renderer.item_calls[6].draw.green_strength == 800 &&
                renderer.item_calls[6].draw.blue_strength == 500 &&
                renderer.item_calls[6].draw.x == 717 &&
                renderer.item_calls[6].draw.y == 289 &&
                renderer.item_calls[7].pattern == 107 &&
                renderer.item_calls[7].draw.palette == 60 &&
                renderer.item_calls[7].draw.x == 777 &&
                renderer.item_calls[7].draw.y == 289,
            "Ostare's drops do not use the retail ground CAF or depth order.")) {
        return false;
    }
    world.advanceConversation();
    if (!check(
            world.conversationActive() &&
                world.conversationMessageId() == 1000004,
            "Ostare's last opening message did not follow retail.")) {
        return false;
    }
    world.advanceConversation();
    if (!check(
            !world.conversationActive() &&
                world.conversationActorId() == -1,
            "Ostare's opening conversation did not release world control.")) {
        return false;
    }

    if (!check(
            world.commandWorldInteraction(747, 269) &&
                world.conversationActive() &&
                world.conversationActorId() == 0 &&
                world.conversationMessageId() == 1000005 &&
                world.conversationText().rfind(
                    "There is no new information so far", 0) == 0,
            "Ostare's repeat interaction did not query the player level.")) {
        return false;
    }
    world.advanceConversation();
    if (!check(
            world.conversationActive() &&
                world.conversationMessageId() == 1000006,
            "Ostare's repeat callback did not show its second message.")) {
        return false;
    }
    world.advanceConversation();
    if (!check(
            !world.conversationActive() &&
                world.conversationActorId() == -1,
            "Ostare's repeat conversation did not close cleanly.")) {
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
    return testGroundItemCreation() &&
                   testFixture() &&
                   testMalformedData() &&
                   testRetailRemoteTown()
               ? 0
               : 1;
}
