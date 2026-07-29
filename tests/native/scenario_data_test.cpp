#include "gapi/gapi.hpp"
#include "render/conversation_layout.hpp"
#include "render/gameplay_overlay_renderer.hpp"
#include "render/gameplay_renderer.hpp"
#include "world/ground_item.hpp"
#include "world/movement_controller.hpp"
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

bool updateUntilConversation(
    osf::WorldScene& world,
    std::int32_t maximum_updates = 2000) {
    for (std::int32_t update = 0;
         update < maximum_updates &&
         !world.conversationActive();
         ++update) {
        world.update();
    }
    return world.conversationActive();
}

bool findNpcPointerPoint(
    osf::WorldScene& world,
    std::int32_t npc_id,
    osf::ScreenPosition& point) {
    const auto found = std::find_if(
        world.npcs().begin(),
        world.npcs().end(),
        [npc_id](const osf::NpcActor& npc) {
            return npc.id() == npc_id;
        });
    if (found == world.npcs().end()) {
        return false;
    }
    const osf::ScreenPosition anchor =
        osf::calculateRealPosition(found->position());
    for (std::int32_t y = -found->labelHeight();
         y <= 16;
         ++y) {
        for (std::int32_t x = -48; x <= 48; ++x) {
            point = {
                anchor.x - world.cameraScreenX() + x,
                anchor.y - world.cameraScreenY() + y,
            };
            world.updatePointerHover(point.x, point.y);
            if (world.hoveredNpcId() == npc_id) {
                return true;
            }
        }
    }
    return false;
}

bool findGroundItemPointerPoint(
    osf::WorldScene& world,
    std::int32_t item_id,
    osf::ScreenPosition& point) {
    const auto found = std::find_if(
        world.groundItems().begin(),
        world.groundItems().end(),
        [item_id](const osf::GroundItem& item) {
            return item.id == item_id;
        });
    if (found == world.groundItems().end()) {
        return false;
    }
    const osf::ScreenPosition anchor =
        osf::calculateRealPosition(found->position);
    for (std::int32_t y = -64; y <= 32; ++y) {
        for (std::int32_t x = -64; x <= 64; ++x) {
            point = {
                anchor.x - world.cameraScreenX() + x,
                anchor.y - world.cameraScreenY() + y,
            };
            world.updatePointerHover(point.x, point.y);
            if (world.hoveredGroundItemId() == item_id) {
                return true;
            }
        }
    }
    return false;
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

bool testConversationChoiceMarkup() {
    const osf::ConversationTextLayout layout =
        osf::layoutConversationText(
            "Dune\r\n~Check Status~\n  ~QUIT~",
            true);
    return check(
        layout.text == "Dune\nCheck Status\n  QUIT" &&
            layout.choices.size() == 2 &&
            layout.choices[0].index == 0 &&
            layout.choices[0].line == 1 &&
            layout.choices[0].column == 0 &&
            layout.choices[0].length == 12 &&
            layout.choices[0].byte_offset == 5 &&
            layout.choices[0].byte_length == 12 &&
            layout.choices[1].index == 1 &&
            layout.choices[1].line == 2 &&
            layout.choices[1].column == 2 &&
            layout.choices[1].length == 4 &&
            layout.choices[1].byte_offset == 20 &&
            layout.choices[1].byte_length == 4,
        "Retail companion choice markup was not removed and ranged.");
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
    osf::PlayerLoadRequest player_request;
    player_request.name = "Mina";
    if (!check(
            world.loadInitialScenario(
                data_root, player_request, &error),
            "Remote Town could not be loaded through Scenario.Mct.")) {
        std::cerr << error << '\n';
        return false;
    }
    if (!check(
        world.scenario().title() == "Remote Town" &&
            world.playerWorldX() == 89898 &&
            world.playerWorldY() == 2811 &&
            world.playerDirection() == 3 &&
            world.playerData().name() == "Mina" &&
            world.playerData().level() == 1 &&
            world.playerData().baseMaximumLife() == 140 &&
            world.playerData().baseMaximumMana() == 160 &&
            world.musicTrack() == 0 &&
            world.npcs().size() == scenario.people().size() &&
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
            world.npcs()[0].partEnabled(6) &&
            world.npcs()[1].id() == 1 &&
            world.npcs()[1].resourceId() == 8 &&
            world.npcs()[1].name() == "Malse" &&
            world.npcs()[2].id() == 2 &&
            world.npcs()[2].resourceId() == 9 &&
            world.npcs()[2].name() == "Syria" &&
            world.npcs()[3].id() == 10000 &&
            world.npcs()[3].resourceId() == 1000000 &&
            world.npcs()[3].name() == "Kerberos" &&
            world.npcs()[6].id() == 10003 &&
            world.npcs()[6].resourceId() == 1000001 &&
            world.npcs()[6].name() == "Harley",
        "WorldScene did not build the Remote Town people table.")) {
        return false;
    }

    const auto& remote_objects = world.objectMap().objects();
    if (!check(
            remote_objects.size() == 279 &&
                remote_objects[138].pattern_set == 5 &&
                remote_objects[138].pattern == 1 &&
                remote_objects[206].pattern_set == 14 &&
                remote_objects[206].pattern == 4,
            "The Remote Town house or west-wall fixture changed.")) {
        return false;
    }
    const auto actorDrawsBefore =
        [](const osf::MapObject& object,
           osf::WorldPosition actor_position) {
            std::vector<osf::DisplayOrderEntry> entries{
                {
                    0,
                    {object.world_x, object.world_y},
                    object.judgement,
                    object.status,
                },
                {
                    1,
                    actor_position,
                    {-80, -80, 79, 79},
                    0,
                },
            };
            osf::sortDisplayObjects(entries);
            return entries.size() == 2 &&
                   entries[0].source_index == 1 &&
                   entries[1].source_index == 0;
        };
    if (!check(
            actorDrawsBefore(
                remote_objects[138], {88500, -1500}) &&
                actorDrawsBefore(
                    remote_objects[206], {86880, 3700}),
            "Remote Town's house or wall did not occlude the player "
            "through its retail judgement rectangle.")) {
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
    osf::ScreenPosition ostare_pointer;
    if (!check(
            findNpcPointerPoint(
                world, 0, ostare_pointer),
            "Ostare has no opaque retail pointer cell.")) {
        return false;
    }
    osf::renderWorld(renderer, world, 500, &font);
    if (!check(
            world.hoveredNpcId() == 0 &&
                renderer.calls.size() == 3 &&
                renderer.calls[1].draw.red_strength == 1300 &&
                renderer.calls[1].draw.green_strength == 1300 &&
                renderer.calls[1].draw.blue_strength == 1300 &&
                renderer.rectangles.size() == 5 &&
                renderer.rectangles[0].x == 725 &&
                renderer.rectangles[0].y == 187 &&
                renderer.rectangles[0].width == 41 &&
                renderer.rectangles[0].height == 15 &&
                renderer.rectangles[0].opacity == 500 &&
                renderer.rectangles[1].opacity == 300 &&
                renderer.rectangles[1].color.red == 255 &&
                renderer.rectangles[1].x ==
                    ostare_pointer.x - 16 &&
                renderer.rectangles[1].y ==
                    ostare_pointer.y - 16 &&
                renderer.rectangles[1].width == 33 &&
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
    const bool ostare_click =
        world.commandWorldInteraction(
            ostare_pointer.x, ostare_pointer.y);
    const bool ostare_approached =
        world.interactionPending() &&
        !world.conversationActive();
    updateUntilConversation(world);
    if (!check(
            world.scenarioScript().messages().size() == 61 &&
                ostare_click &&
                ostare_approached &&
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
                renderer.speech_calls[1].pattern == 2 &&
                renderer.speech_calls[2].pattern == 1 &&
                renderer.speech_calls[3].pattern == 3 &&
                renderer.speech_calls[4].pattern == 4 &&
                renderer.speech_calls[1].draw.y ==
                    renderer.speech_calls[0].draw.y &&
                renderer.speech_calls[2].draw.x ==
                    renderer.speech_calls[0].draw.x &&
                renderer.speech_calls[3].draw.x ==
                    renderer.speech_calls[1].draw.x &&
                renderer.speech_calls[3].draw.y ==
                    renderer.speech_calls[2].draw.y &&
                renderer.speech_calls[4].draw.y ==
                    renderer.speech_calls[2].draw.y + 5 &&
                renderer.rectangles.size() == 13 &&
                renderer.rectangles[0].x ==
                    renderer.speech_calls[0].draw.x + 4 &&
                renderer.rectangles[0].y ==
                    renderer.speech_calls[0].draw.y + 4 &&
                renderer.rectangles[0].width == 378 &&
                renderer.rectangles[0].height == 78 &&
                renderer.rectangles[0].color.red == 255 &&
                renderer.rectangles[1].x ==
                    renderer.speech_calls[0].draw.x + 9 &&
                renderer.rectangles[1].y ==
                    renderer.speech_calls[0].draw.y &&
                renderer.rectangles[1].width == 368 &&
                renderer.rectangles[1].height == 2 &&
                renderer.rectangles[2].color.red == 160 &&
                renderer.rectangles[3].color.red == 224 &&
                renderer.rectangles[7].x ==
                    renderer.speech_calls[0].draw.x &&
                renderer.rectangles[7].y ==
                    renderer.speech_calls[0].draw.y + 9 &&
                renderer.rectangles[10].x ==
                    renderer.speech_calls[1].draw.x + 7 &&
                renderer.text_calls.size() == 1 &&
                renderer.text_calls[0].draw.x ==
                    renderer.speech_calls[0].draw.x + 13 &&
                renderer.text_calls[0].draw.y ==
                    renderer.speech_calls[0].draw.y + 13 &&
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
                renderer.item_calls[4].draw.x ==
                    renderer.item_calls[0].draw.x &&
                renderer.item_calls[4].draw.y ==
                    renderer.item_calls[0].draw.y &&
                renderer.item_calls[5].pattern == 77 &&
                renderer.item_calls[5].draw.palette == 0 &&
                renderer.item_calls[5].draw.x ==
                    renderer.item_calls[1].draw.x &&
                renderer.item_calls[5].draw.y ==
                    renderer.item_calls[1].draw.y &&
                renderer.item_calls[6].pattern == 82 &&
                renderer.item_calls[6].draw.palette == 10 &&
                renderer.item_calls[6].draw.red_strength == 900 &&
                renderer.item_calls[6].draw.green_strength == 800 &&
                renderer.item_calls[6].draw.blue_strength == 500 &&
                renderer.item_calls[6].draw.x ==
                    renderer.item_calls[2].draw.x &&
                renderer.item_calls[6].draw.y ==
                    renderer.item_calls[2].draw.y &&
                renderer.item_calls[7].pattern == 107 &&
                renderer.item_calls[7].draw.palette == 60 &&
                renderer.item_calls[7].draw.x ==
                    renderer.item_calls[3].draw.x &&
                renderer.item_calls[7].draw.y ==
                    renderer.item_calls[3].draw.y,
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
            findNpcPointerPoint(
                world, 0, ostare_pointer),
            "Ostare lost his opaque pointer cells.")) {
        return false;
    }
    if (!check(
            world.commandWorldInteraction(
                ostare_pointer.x, ostare_pointer.y) &&
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

    const std::int32_t short_sword_id =
        world.groundItems().front().id;
    osf::ScreenPosition short_sword_pointer;
    if (!check(
            findGroundItemPointerPoint(
                world,
                short_sword_id,
                short_sword_pointer),
            "The Short Sword has no opaque retail pointer cell.")) {
        return false;
    }
    renderer.item_calls.clear();
    renderer.text_calls.clear();
    renderer.rectangles.clear();
    osf::renderWorld(renderer, world, 500, &font);
    const bool item_tinted = std::any_of(
        renderer.item_calls.begin(),
        renderer.item_calls.end(),
        [](const NpcPatternCall& call) {
            return !call.shadow &&
                   call.draw.red_strength == 1300 &&
                   call.draw.green_strength == 1300 &&
                   call.draw.blue_strength == 1300;
        });
    if (!check(
            world.hoveredGroundItemId() ==
                    short_sword_id &&
                item_tinted &&
                renderer.text_calls.size() == 2 &&
                renderer.text_calls[0].text ==
                    "Short Sword" &&
                renderer.rectangles.size() == 5 &&
                renderer.rectangles[1].color.red == 224 &&
                renderer.rectangles[1].color.green == 224 &&
                renderer.rectangles[1].color.blue == 0 &&
                renderer.rectangles[1].opacity == 300,
            "Ground-item hover feedback differs from retail.")) {
        return false;
    }
    const bool short_sword_click =
        world.commandWorldInteraction(
            short_sword_pointer.x,
            short_sword_pointer.y);
    for (std::int32_t update = 0;
         update < 2000 &&
         world.groundItems().size() == 4;
         ++update) {
        world.update();
    }
    if (!check(
            short_sword_click &&
                world.groundItems().size() == 3 &&
                world.playerInventory().items().size() == 1 &&
                world.playerInventory().items()[0].category == 0 &&
                world.playerInventory()
                        .items()[0]
                        .definition_id == 0 &&
                world.playerInventory().items()[0].quantity == 1,
            "The retail approach-and-pickup path did not transfer "
            "the Short Sword into player inventory.")) {
        return false;
    }

    const osf::NpcActor& malse = world.npcs()[1];
    osf::ScreenPosition malse_pointer;
    if (!check(
            findNpcPointerPoint(
                world, malse.id(), malse_pointer),
            "Malse has no opaque retail pointer cell.")) {
        return false;
    }
    const bool malse_click =
        world.commandWorldInteraction(
            malse_pointer.x, malse_pointer.y);
    updateUntilConversation(world);
    if (!check(
            malse_click &&
                world.conversationActive() &&
                world.conversationActorId() == 1 &&
                world.conversationMessageId() == 1000019,
            "Malse's actor did not enter its retail status-zero script.")) {
        std::cerr
            << "Player: "
            << world.playerWorldX() << ", "
            << world.playerWorldY()
            << "; Malse: "
            << malse.position().x << ", "
            << malse.position().y
            << "; pending: "
            << world.interactionPending() << '\n';
        return false;
    }
    world.advanceConversation();
    if (!check(
            world.conversationActive() &&
                world.conversationMessageId() == 1000020,
            "Malse's first message callback did not follow retail.")) {
        return false;
    }
    world.advanceConversation();
    if (!check(
            !world.conversationActive() &&
                world.conversationActorId() == -1,
            "Malse's opening conversation did not release world control.")) {
        return false;
    }

    const osf::NpcActor& syria = world.npcs()[2];
    const osf::ScreenPosition syria_anchor =
        osf::calculateRealPosition(syria.position());
    const std::int32_t syria_screen_x =
        syria_anchor.x - world.cameraScreenX();
    const std::int32_t syria_screen_y =
        syria_anchor.y - world.cameraScreenY();
    const bool syria_click =
        world.commandWorldInteraction(
            syria_screen_x, syria_screen_y);
    updateUntilConversation(world);
    if (!check(
            syria_click &&
                world.conversationActive() &&
                world.conversationActorId() == 2 &&
                world.conversationMessageId() == 1000040,
            "Syria's actor did not enter its retail status-zero script.")) {
        std::cerr
            << "Player: "
            << world.playerWorldX() << ", "
            << world.playerWorldY()
            << "; Syria: "
            << syria.position().x << ", "
            << syria.position().y
            << "; pending: "
            << world.interactionPending() << '\n';
        return false;
    }
    world.advanceConversation();
    if (!check(
            world.conversationActive() &&
                world.conversationActorId() == 2 &&
                world.conversationMessageId() == 1000041 &&
                world.quests().state(0) == 1 &&
                world.quests().lastCue() ==
                    osf::QuestCue::updated &&
                world.quests().notice().quest_id == 0 &&
                world.quests().notice().counter == 600,
            "Syria's callback did not apply its retail quest update.")) {
        return false;
    }
    world.advanceConversation();
    if (!check(
            !world.conversationActive() &&
                world.conversationActorId() == -1,
            "Syria's opening conversation did not release world control.")) {
        return false;
    }

    osf::WorldScene companion_world;
    if (!check(
            companion_world.loadInitialScenario(
                data_root, osf::PlayerLoadRequest{}, &error),
            "Remote Town could not be reloaded for the companion check.")) {
        return false;
    }
    constexpr osf::ObjectBounds player_bounds{
        -80, -80, 79, 79};
    const osf::NpcActor& malse_route_target =
        companion_world.npcs()[1];
    std::vector<osf::MovementBlocker> town_actor_blockers;
    for (const osf::NpcActor& npc : companion_world.npcs()) {
        town_actor_blockers.push_back({
            npc.id(),
            npc.position(),
            npc.judgement(),
        });
    }
    osf::MovementController actor_route_controller;
    osf::WorldPosition actor_route_position{90933, 1842};
    for (std::int32_t update = 0;
         update < 2000 &&
         osf::distanceBetweenBounds(
             actor_route_position,
             player_bounds,
             malse_route_target.position(),
             malse_route_target.judgement()) > 159;
         ++update) {
        actor_route_position =
            actor_route_controller.advance(
                companion_world.ground(),
                companion_world.objectMap(),
                player_bounds,
                actor_route_position,
                malse_route_target.position(),
                20,
                &town_actor_blockers).position;
    }
    if (!check(
            osf::distanceBetweenBounds(
                actor_route_position,
                player_bounds,
                malse_route_target.position(),
                malse_route_target.judgement()) <= 159,
            "The Remote Town sacks route did not clear scenery and "
            "live actor judgement.")) {
        return false;
    }

    const osf::NpcActor& kerberos = companion_world.npcs()[3];
    constexpr osf::WorldPosition sacks_route_start{
        89800, 1450};
    constexpr osf::WorldPosition sacks_route_destination{
        91800, 1450};
    if (!check(
            !osf::positionIsWalkable(
                companion_world.ground(),
                companion_world.objectMap(),
                {90700, 1450},
                player_bounds),
            "The Remote Town sacks regression no longer crosses their "
            "blocked ground footprint.")) {
        return false;
    }
    osf::MovementController sacks_controller;
    osf::WorldPosition sacks_position = sacks_route_start;
    std::int32_t greatest_sacks_detour = 0;
    for (std::int32_t update = 0;
         update < 500 &&
         (sacks_position.x != sacks_route_destination.x ||
          sacks_position.y != sacks_route_destination.y);
         ++update) {
        const osf::MovementStepResult step =
            sacks_controller.advance(
                companion_world.ground(),
                companion_world.objectMap(),
                player_bounds,
                sacks_position,
                sacks_route_destination,
                20);
        sacks_position = step.position;
        greatest_sacks_detour = std::max(
            greatest_sacks_detour,
            std::abs(
                sacks_position.y -
                sacks_route_start.y));
    }
    if (!check(
            sacks_position.x == sacks_route_destination.x &&
                sacks_position.y ==
                    sacks_route_destination.y &&
                greatest_sacks_detour > 100,
            "Player navigation did not follow the full edge of the "
            "Remote Town sacks.")) {
        return false;
    }
    constexpr osf::WorldPosition town_routes[] = {
        {92500, 500},
        {91200, 500},
        {93000, 3000},
        {88700, 500},
    };
    for (const osf::WorldPosition destination : town_routes) {
        osf::MovementController controller;
        osf::WorldPosition position{89898, 2811};
        for (std::int32_t update = 0;
             update < 1000 &&
             (position.x != destination.x ||
              position.y != destination.y);
             ++update) {
            position = controller.advance(
                companion_world.ground(),
                companion_world.objectMap(),
                player_bounds,
                position,
                destination,
                20).position;
        }
        if (!check(
                position.x == destination.x &&
                    position.y == destination.y,
                "The movement controller did not retry direct movement "
                "between separate Remote Town obstacles.")) {
            std::cerr << "Destination: "
                      << destination.x << ", "
                      << destination.y << "; position: "
                      << position.x << ", "
                      << position.y << '\n';
            return false;
        }
    }
    // The retail controller follows nearby obstacle edges; it is not a
    // whole-map route planner. Keep this interaction test to camera-sized
    // movement legs, like actual play does.
    constexpr osf::WorldPosition kerberos_approach[] = {
        {92000, 500},
        {92000, -1000},
        {93200, -3200},
        {89900, -3200},
    };
    for (const osf::WorldPosition waypoint : kerberos_approach) {
        const osf::ScreenPosition anchor =
            osf::calculateRealPosition(waypoint);
        companion_world.commandPlayerMovement(
            anchor.x - companion_world.cameraScreenX(),
            anchor.y - companion_world.cameraScreenY());
        for (std::int32_t update = 0;
             update < 2000 &&
             companion_world.playerMotion() !=
                 osf::PlayerMotion::idle;
             ++update) {
            companion_world.update();
        }
    }
    const osf::ScreenPosition kerberos_anchor =
        osf::calculateRealPosition(kerberos.position());
    const bool kerberos_click =
        companion_world.commandWorldInteraction(
            kerberos_anchor.x -
                companion_world.cameraScreenX(),
            kerberos_anchor.y -
                companion_world.cameraScreenY());
    const bool kerberos_approached =
        companion_world.interactionPending();
    if (!check(
            kerberos_click &&
                kerberos_approached &&
                updateUntilConversation(companion_world, 5000) &&
                companion_world.conversationActorId() == 10000 &&
                companion_world.conversationRequiresSelection() &&
                companion_world.conversationInitialSelection() == 3,
            "The retail movement controller did not approach Kerberos "
            "and open his choice message.")) {
        std::cerr
            << "Player: "
            << companion_world.playerWorldX() << ", "
            << companion_world.playerWorldY()
            << "; Kerberos: "
            << kerberos.position().x << ", "
            << kerberos.position().y << '\n';
        return false;
    }

    NpcRecordingBackend choice_renderer;
    choice_renderer.speech =
        &companion_world.speechPatterns();
    osf::renderWorld(
        choice_renderer, companion_world, 500, &font);
    const osf::ConversationTextLayout choice_layout =
        osf::layoutConversationText(
            companion_world.conversationText(), true);
    const auto quit = std::find_if(
        choice_layout.choices.begin(),
        choice_layout.choices.end(),
        [](const osf::ConversationChoiceSpan& choice) {
            return choice.index == 3;
        });
    const auto status = std::find_if(
        choice_layout.choices.begin(),
        choice_layout.choices.end(),
        [](const osf::ConversationChoiceSpan& choice) {
            return choice.index == 0;
        });
    if (!check(
            !choice_renderer.text_calls.empty() &&
                quit != choice_layout.choices.end() &&
                status != choice_layout.choices.end(),
            "Kerberos's rendered choice message is missing a range.")) {
        return false;
    }
    const auto choice_text = std::find_if(
        choice_renderer.text_calls.begin(),
        choice_renderer.text_calls.end(),
        [&choice_layout](const TextCall& call) {
            return call.text == choice_layout.text;
        });
    const auto selected_quit = std::find_if(
        choice_renderer.text_calls.begin(),
        choice_renderer.text_calls.end(),
        [](const TextCall& call) {
            return call.text == "QUIT" &&
                   call.draw.color.red == 255 &&
                   call.draw.color.green == 0 &&
                   call.draw.color.blue == 0;
        });
    const auto unselected_status = std::find_if(
        choice_renderer.text_calls.begin(),
        choice_renderer.text_calls.end(),
        [](const TextCall& call) {
            return call.text == "Check Status" &&
                   call.draw.color.red == 96 &&
                   call.draw.color.green == 96 &&
                   call.draw.color.blue == 96;
        });
    if (!check(
            choice_text != choice_renderer.text_calls.end() &&
                selected_quit != choice_renderer.text_calls.end() &&
                unselected_status !=
                    choice_renderer.text_calls.end() &&
                companion_world.conversationSelectedOption() == 3,
            "The script-selected companion choice did not use the "
            "retail red and gray colors.")) {
        return false;
    }
    const osf::gapi::NjpPattern& font_pattern =
        font.patterns().front();
    const std::int32_t cell_width =
        font_pattern.width / 16;
    const std::int32_t cell_height =
        font_pattern.height / 16;
    const std::int32_t status_x =
        choice_text->draw.x +
        status->column * cell_width +
        cell_width / 2;
    const std::int32_t status_y =
        choice_text->draw.y +
        status->line * cell_height +
        cell_height / 2;
    companion_world.selectConversationOption(
        osf::conversationChoiceAtScreenPosition(
            companion_world,
            font,
            companion_world.cameraScreenX(),
            companion_world.cameraScreenY(),
            status_x,
            status_y));
    companion_world.selectConversationOption(-1);
    NpcRecordingBackend hovered_choice_renderer;
    hovered_choice_renderer.speech =
        &companion_world.speechPatterns();
    osf::renderWorld(
        hovered_choice_renderer,
        companion_world,
        500,
        &font);
    const auto hovered_status = std::find_if(
        hovered_choice_renderer.text_calls.begin(),
        hovered_choice_renderer.text_calls.end(),
        [](const TextCall& call) {
            return call.text == "Check Status" &&
                   call.draw.color.red == 255 &&
                   call.draw.color.green == 0 &&
                   call.draw.color.blue == 0;
        });
    const auto unselected_quit = std::find_if(
        hovered_choice_renderer.text_calls.begin(),
        hovered_choice_renderer.text_calls.end(),
        [](const TextCall& call) {
            return call.text == "QUIT" &&
                   call.draw.color.red == 96 &&
                   call.draw.color.green == 96 &&
                   call.draw.color.blue == 96;
        });
    if (!check(
            companion_world.conversationSelectedOption() == 0 &&
                hovered_status !=
                    hovered_choice_renderer.text_calls.end() &&
                unselected_quit !=
                    hovered_choice_renderer.text_calls.end(),
            "Pointer selection did not move the retail red highlight "
            "between companion choices.")) {
        return false;
    }
    const std::int32_t quit_x =
        choice_text->draw.x +
        quit->column * cell_width +
        cell_width / 2;
    const std::int32_t quit_y =
        choice_text->draw.y +
        quit->line * cell_height +
        cell_height / 2;
    const std::int32_t selected =
        osf::conversationChoiceAtScreenPosition(
            companion_world,
            font,
            companion_world.cameraScreenX(),
            companion_world.cameraScreenY(),
            quit_x,
            quit_y);
    companion_world.selectConversationOption(selected);
    companion_world.chooseConversationOption(selected);
    if (!check(
            selected == 3 &&
                !companion_world.conversationActive() &&
                companion_world.conversationActorId() == -1,
            "Kerberos's rendered QUIT choice was not clickable.")) {
        return false;
    }

    osf::WorldScene harley_world;
    if (!check(
            harley_world.loadInitialScenario(
                data_root, osf::PlayerLoadRequest{}, &error),
            "Remote Town could not be reloaded for Harley's dialogue.")) {
        return false;
    }
    const osf::NpcActor& harley = harley_world.npcs()[6];
    const osf::ScreenPosition harley_anchor =
        osf::calculateRealPosition(harley.position());
    if (!check(
            harley_world.commandWorldInteraction(
                harley_anchor.x -
                    harley_world.cameraScreenX(),
                harley_anchor.y -
                    harley_world.cameraScreenY()) &&
                updateUntilConversation(harley_world, 5000) &&
                harley_world.conversationMessageId() == 1000056 &&
                harley_world.conversationRequiresSelection(),
            "Harley's choice menu did not open through live world "
            "interaction.")) {
        return false;
    }
    harley_world.chooseConversationOption(1);
    if (!check(
            harley_world.conversationActive() &&
                harley_world.conversationMessageId() == 1000057 &&
                !harley_world.conversationRequiresSelection(),
            "Harley's first explanation line remained stuck in choice "
            "mode.")) {
        return false;
    }
    harley_world.advanceConversation();
    if (!check(
            harley_world.conversationActive() &&
                harley_world.conversationMessageId() == 1000058 &&
                !harley_world.conversationRequiresSelection(),
            "Harley's explanation did not advance to its second line.")) {
        return false;
    }
    harley_world.advanceConversation();
    if (!check(
            !harley_world.conversationActive() &&
                harley_world.conversationActorId() == -1,
            "Harley was not released after his explanation.")) {
        return false;
    }

    osf::WorldScene wander_world;
    if (!check(
            wander_world.loadInitialScenario(
                data_root, osf::PlayerLoadRequest{}, &error),
            "Remote Town could not be reloaded for the wander check.")) {
        return false;
    }
    wander_world.update();
    if (!check(
            wander_world.npcs()[0].animationFrame() == 0,
            "Ostare skipped the first retail idle frame.")) {
        return false;
    }
    wander_world.update();
    if (!check(
            wander_world.npcs()[0].animationFrame() == 1,
            "Ostare's idle animation does not advance at game-update cadence.")) {
        return false;
    }

    for (std::int32_t update = 2; update < 30; ++update) {
        wander_world.update();
    }
    if (!check(
            wander_world.npcs()[0].position().x == 91467 &&
                wander_world.npcs()[0].position().y == 1532 &&
                wander_world.npcs()[0].animationChart() == 0,
            "Ostare did not keep the retail 30-update idle pause.")) {
        return false;
    }
    wander_world.update();
    const osf::WorldPosition walking_position =
        wander_world.npcs()[0].position();
    return check(
        wander_world.npcs()[0].animationChart() == 1 &&
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
                   testConversationChoiceMarkup() &&
                   testFixture() &&
                   testMalformedData() &&
                   testRetailRemoteTown()
               ? 0
               : 1;
}
