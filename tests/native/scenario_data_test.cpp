#include "core/retail_random.hpp"
#include "gapi/gapi.hpp"
#include "libs/RKC_RPGSCRN/rkc_rpgscrn.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "ui/conversation_layout.hpp"
#include "ui/player_level_up_notice_input.hpp"
#include "ui/player_level_up_notice_layout.hpp"
#include "render/enemy_nameplate_renderer.hpp"
#include "render/gameplay_renderer.hpp"
#include "render/player_level_up_notice_renderer.hpp"
#include "resources/character_visual_resource.hpp"
#include "world/actor_direction.hpp"
#include "world/enemy_effect_impact.hpp"
#include "world/ground_item.hpp"
#include "world/movement_controller.hpp"
#include "world/npc_script_action.hpp"
#include "world/retail_save_file.hpp"
#include "world/scenario_data.hpp"
#include "world/script/scenario_effect_command.hpp"
#include "world/script/scenario_placed_effect_command.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <set>
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

bool testNpcScriptAction() {
    osf::NpcScriptActionController action;
    if (!check(
            action.start(4, -1, -1, -1),
            "A valid retail PEOPLE one-shot action was rejected.")) {
        return false;
    }
    const osf::NpcScriptActionUpdate first = action.update(3);
    const osf::NpcScriptActionUpdate second = action.update(3);
    const osf::NpcScriptActionUpdate third = action.update(3);
    if (!check(
            first.handled && first.action == 4 && first.frame == 0 &&
                !first.completed && second.frame == 1 &&
                !second.completed && third.frame == 2 &&
                third.completed && !action.active() &&
                action.action() == 1,
            "The PEOPLE one-shot action did not preserve its retail "
            "first and final frames.")) {
        return false;
    }
    if (!check(
            action.start(4, 1, 1, 2),
            "A valid retail PEOPLE repeated action was rejected.")) {
        return false;
    }
    std::array<std::int32_t, 5> frames{};
    for (std::int32_t& frame : frames) {
        frame = action.update(4).frame;
    }
    return check(
        frames == std::array<std::int32_t, 5>{0, 1, 2, 1, 2} &&
            action.active() &&
            !action.start(3, -1, -1, -1),
        "The PEOPLE repeated-action restart and range validation differ "
        "from retail.");
}

bool testScenarioEffectCommand() {
    const std::vector<std::int32_t> arguments{
        1000,
        2000,
        2,
        90,
        60,
        123,
        150,
        50,
        -1,
        2,
        42,
        43,
        41,
        72,
    };
    osf::CombatEffectSpawnRequest request;
    if (!check(
            osf::makeScenarioEffectRequest(
                arguments, 7, request),
            "The retail scenario-effect descriptor was rejected.")) {
        return false;
    }
    if (!check(
            request.valid && request.effect_number == 2 &&
                request.owner_kind == 0 &&
                request.source_character_number == -1 &&
                request.target_kind == 19 &&
                request.target_identifier == -1 &&
                request.constructor_value_6 == 60 &&
                request.constructor_value_7 == 150 &&
                std::abs(
                    request.direction_radians -
                    90.0 * osf::kRetailRadiansPerDegree) <
                    0.000001 &&
                request.has_explicit_origin &&
                request.origin.x == 1000 &&
                request.origin.y == 1951 &&
                !request.has_source_judgement &&
                request.constructor_value_12 == 0 &&
                request.has_packet && request.packet_kind == 8 &&
                request.instance_identifier == -1 &&
                request.constructor_value_16 == 0 &&
                request.constructor_value_17 == 0 &&
                request.constructor_value_18 == 0 &&
                request.constructor_value_19 == 0 &&
                request.constructor_value_20 == 0 &&
                request.constructor_value_21 == 200 &&
                request.constructor_value_22 == 0,
            "Opcode 30 did not reproduce the retail 22-field effect "
            "request.")) {
        return false;
    }
    const osf::CombatPacket& packet = request.packet;
    if (!check(
            packet[0] == 2 && packet[2] == -1 &&
                packet[4] == 123 && packet[32] == -1 &&
                packet[34] == 21003 && packet[35] == 8 &&
                packet[36] == 9999 && packet[37] == 1 &&
                packet[40] == 41 && packet[41] == 42 &&
                packet[42] == 0 &&
                packet[43] == 43 && packet[72] == 72 &&
                packet[73] == -1 && packet[74] == -1 &&
                packet[75] == 8 &&
                packet.written_words.count() == 33,
            "Opcode 30 did not reproduce its retail combat packet.")) {
        return false;
    }

    std::vector<std::int32_t> alternate = arguments;
    alternate[2] = 0;
    return check(
        osf::makeScenarioEffectRequest(alternate, 5, request) &&
            request.packet[34] == 21009 &&
            !osf::makeScenarioEffectRequest(
                std::vector<std::int32_t>(13), 0, request),
        "Opcode 30 did not preserve its alternate impact family or "
        "operand count.");
}

bool testScenarioPlacedEffectCommand() {
    const std::vector<std::int32_t> arguments{
        20009,
        1234,
        5678,
        150,
        -1,
        -1,
        1,
    };
    osf::CombatEffectSpawnRequest request;
    if (!check(
            osf::makeScenarioPlacedEffectRequest(
                arguments, request),
            "The retail placed-effect descriptor was rejected.")) {
        return false;
    }
    if (!check(
            request.valid && request.effect_number == 20009 &&
                request.owner_kind == 0 &&
                request.source_character_number == 0 &&
                request.target_kind == 0 &&
                request.target_identifier == 0 &&
                request.constructor_value_6 == 0 &&
                request.constructor_value_7 == 150 &&
                request.direction_radians == 0.0 &&
                request.has_explicit_origin &&
                request.origin.x == 1234 &&
                request.origin.y == 5678 &&
                request.has_source_judgement &&
                request.source_judgement.left == 0 &&
                request.source_judgement.top == 0 &&
                request.source_judgement.right == -1 &&
                request.source_judgement.bottom == 1 &&
                request.constructor_value_12 == 0 &&
                !request.has_packet && request.packet_kind == 8 &&
                request.instance_identifier == -1 &&
                request.constructor_value_16 == 0 &&
                request.constructor_value_17 == 0 &&
                request.constructor_value_18 == 0 &&
                request.constructor_value_19 == 0 &&
                request.constructor_value_20 == 0 &&
                request.constructor_value_21 == 200 &&
                request.constructor_value_22 == 0,
            "Opcode 36 did not reproduce the retail 22-field effect "
            "request.")) {
        return false;
    }
    return check(
        osf::retailCombatEffectResourceId(20007) == 11000005 &&
            osf::retailCombatEffectResourceId(20008) == 11000006 &&
            osf::retailCombatEffectResourceId(20009) == 11000007 &&
            !osf::makeScenarioPlacedEffectRequest(
                std::vector<std::int32_t>(6), request),
        "Opcode 36 did not preserve its retail OPTION resources or "
        "operand count.");
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

bool findScenarioObjectPointerPoint(
    osf::WorldScene& world,
    std::int32_t object_id,
    osf::ScreenPosition& point) {
    const auto found = std::find_if(
        world.scenarioObjects().begin(),
        world.scenarioObjects().end(),
        [object_id](const osf::ScenarioObjectActor& object) {
            return object.id() == object_id;
        });
    if (found == world.scenarioObjects().end()) {
        return false;
    }
    const osf::ScreenPosition anchor =
        osf::calculateRealPosition(found->position());
    for (std::int32_t y = -240; y <= 160; ++y) {
        for (std::int32_t x = -240; x <= 240; ++x) {
            point = {
                anchor.x - world.cameraScreenX() + x,
                anchor.y - world.cameraScreenY() + y,
            };
            world.updatePointerHover(point.x, point.y);
            if (world.hoveredScenarioObjectId() == object_id) {
                return true;
            }
        }
    }
    return false;
}

bool findGroundItemRangeOnlyPoint(
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
    const osf::ItemWorldResource* resource =
        world.itemWorldResource(found->resource_id);
    if (!resource) {
        return false;
    }
    const osf::ScreenPosition anchor =
        osf::calculateRealPosition(found->position);
    for (std::int32_t y = -96; y <= 64; ++y) {
        for (std::int32_t x = -96; x <= 96; ++x) {
            point = {
                anchor.x - world.cameraScreenX() + x,
                anchor.y - world.cameraScreenY() + y,
            };
            const bool exact_hit =
                osf::displayAnimationContainsPoint(
                    resource->animation(),
                    resource->patterns(),
                    found->position,
                    found->animation_chart,
                    8,
                    0,
                    [](std::size_t) {
                        return true;
                    },
                    world.cameraScreenX(),
                    world.cameraScreenY(),
                    point,
                    found->height * 20 / 100);
            if (exact_hit) {
                continue;
            }
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
                items[0].item.category == 2 &&
                items[0].item.definition_id == 45 &&
                items[0].item.quantity == 1 &&
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
                items[1].item.quantity == 10000 &&
                items[2].item.quantity == 10000 &&
                items[3].item.quantity == 5000 &&
                items[1].position.x == 300 &&
                items[1].position.y == 200,
            "Retail money splitting or radial placement differs.")) {
        return false;
    }
    osf::GroundItem bouncing = items.front();
    const osf::GroundItemUpdateEvent first_update =
        osf::updateGroundItem(bouncing);
    if (!check(
            first_update ==
                    osf::GroundItemUpdateEvent::none &&
            bouncing.height == 160 &&
                bouncing.vertical_velocity == 1320 &&
                bouncing.bounce_state == 0,
            "A new ground item did not begin its retail drop arc.")) {
        return false;
    }
    osf::GroundItem restarted = bouncing;
    restarted.height = 400;
    restarted.vertical_velocity = -80;
    restarted.vertical_gravity = 100;
    restarted.bounce_state = 2;
    osf::restartGroundItemDrop(restarted);
    if (!check(
            restarted.height == 0 &&
                restarted.vertical_velocity == 1600 &&
                restarted.vertical_gravity == 280 &&
                restarted.bounce_state == 0,
            "Restarting a rejected pickup did not restore the "
            "mode-zero drop state.")) {
        return false;
    }
    std::int32_t impact_count = 0;
    for (std::int32_t update = 1; update < 19; ++update) {
        if (osf::updateGroundItem(bouncing) ==
            osf::GroundItemUpdateEvent::first_impact) {
            ++impact_count;
        }
    }
    if (!check(
            bouncing.height == 0 &&
                bouncing.bounce_state == 2 &&
                impact_count == 1,
            "A ground item did not emit one first impact while settling.")) {
        return false;
    }

    osf::ScenarioItem placed;
    placed.id = 12;
    placed.world_x = -450;
    placed.world_y = 900;
    placed.initial_state_values = {1, 0, 1};
    placed.category = 4;
    placed.definition_id = 0;
    placed.minimum_quantity = 25000;
    placed.maximum_quantity = 25000;
    const std::size_t placed_index = items.size();
    if (!check(
            osf::createScenarioGroundItem(
                items, random, placed) &&
                items.size() == placed_index + 1,
            "A map-authored item could not be created.")) {
        return false;
    }
    osf::GroundItem& authored = items.back();
    if (!check(
            authored.item.quantity == 25000 &&
                authored.position.x == -450 &&
                authored.position.y == 900 &&
                authored.scenario_character_number == 18000012 &&
                authored.height == 0 &&
                authored.vertical_velocity == 0 &&
                authored.bounce_state == 2 &&
                authored.visible() &&
                !authored.pointerEnabled() &&
                authored.judgementEnabled() &&
                osf::updateGroundItem(authored) ==
                    osf::GroundItemUpdateEvent::none,
            "A map-authored item did not enter the settled retail "
            "runtime state.")) {
        return false;
    }

    return check(
        !osf::createGroundItems(
            items, random, 4, 0, {}, 10, 9) &&
            items.size() == placed_index + 1,
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
    const osf::gapi::NjpImage* status_icons = nullptr;
    std::vector<NpcPatternCall> calls;
    std::vector<NpcPatternCall> speech_calls;
    std::vector<NpcPatternCall> item_calls;
    std::vector<NpcPatternCall> status_icon_calls;
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
        } else if (&image == status_icons) {
            status_icon_calls.push_back(
                {false, pattern, draw});
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

bool testPlayerLevelUpNoticeLayout() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    osf::gapi::NjpImage font;
    std::string error;
    if (!check(
            font.load(
                data_root / "System" / "Common" / "Pattern" /
                    "Font01.njp",
                &error),
            "The level-up notice fixture could not load Font01.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerLevelUpNotice notice{"Level 2", 900};
    osf::PlayerLevelUpNoticeLayout centered;
    if (!check(
            osf::buildPlayerLevelUpNoticeLayout(
                notice, font, centered) &&
                centered.x ==
                    (640 - centered.width) / 2 &&
                centered.y ==
                    (416 - centered.height) / 2 &&
                centered.text_x == centered.x + 4 &&
                centered.text_y == centered.y + 4 &&
                !notice.dismissible() &&
                !osf::playerLevelUpNoticeAcceptsPointer(
                    notice,
                    centered.x,
                    centered.y,
                    &font),
            "The level-up notice lost its retail centered geometry "
            "or initial click guard.")) {
        return false;
    }
    NpcRecordingBackend renderer;
    osf::renderPlayerLevelUpNotice(
        renderer, notice, font);
    if (!check(
            renderer.rectangles.size() == 5 &&
                renderer.rectangles[0].x == centered.x &&
                renderer.rectangles[0].y == centered.y &&
                renderer.rectangles[0].width ==
                    centered.width &&
                renderer.rectangles[0].height ==
                    centered.height &&
                renderer.rectangles[0].color.red == 0 &&
                renderer.rectangles[0].color.green == 0 &&
                renderer.rectangles[0].color.blue == 0 &&
                renderer.rectangles[0].opacity == 250 &&
                renderer.rectangles[1].opacity == 500 &&
                renderer.text_calls.size() == 1 &&
                renderer.text_calls[0].draw.x ==
                    centered.text_x &&
                renderer.text_calls[0].draw.y ==
                    centered.text_y,
            "The level-up notice lost its faded background, frame, "
            "padding, or text placement.")) {
        return false;
    }

    notice.counter = 839;
    osf::PlayerLevelUpNoticeLayout sliding;
    if (!check(
            osf::buildPlayerLevelUpNoticeLayout(
                notice, font, sliding) &&
                sliding.x > centered.x &&
                sliding.y < centered.y &&
                notice.dismissible() &&
                osf::playerLevelUpNoticeAcceptsPointer(
                    notice,
                    sliding.x,
                    sliding.y,
                    &font),
            "The level-up notice did not begin its ten-update "
            "upper-right slide.")) {
        return false;
    }

    notice.counter = 830;
    osf::PlayerLevelUpNoticeLayout parked;
    if (!check(
            osf::buildPlayerLevelUpNoticeLayout(
                notice, font, parked) &&
                parked.x == 640 - parked.width &&
                parked.y == 1 &&
                osf::playerLevelUpNoticeContains(
                    parked, parked.x, parked.y) &&
                !osf::playerLevelUpNoticeContains(
                    parked,
                    parked.x + parked.width,
                    parked.y),
            "The level-up notice did not finish at the exact retail "
            "upper-right position or bounds.")) {
        return false;
    }

    notice.counter = 1;
    notice.update();
    return check(
        !notice.active() && notice.text.empty(),
        "The level-up notice did not release itself at update 900.");
#else
    return true;
#endif
}

bool testEnemyNameplatePresentation() {
    osf::gapi::NjpImage font;
    osf::gapi::NjpImage status_icons;
    NpcRecordingBackend renderer;
    renderer.status_icons = &status_icons;
    osf::renderEnemyNameplate(
        renderer,
        font,
        &status_icons,
        {
            "Goblin",
            {224, 224, 224, 255},
            25,
            100,
            2,
            100,
            50,
        });
    return check(
        renderer.rectangles.size() == 3 &&
            renderer.rectangles[0].x == 71 &&
            renderer.rectangles[0].y == 47 &&
            renderer.rectangles[0].width == 56 &&
            renderer.rectangles[0].height == 18 &&
            renderer.rectangles[0].opacity == 800 &&
            renderer.rectangles[1].x == 72 &&
            renderer.rectangles[1].width == 13 &&
            renderer.rectangles[1].height == 16 &&
            renderer.rectangles[1].color.red == 128 &&
            renderer.rectangles[1].color.green == 32 &&
            renderer.rectangles[1].opacity == 500 &&
            renderer.rectangles[2].x == 85 &&
            renderer.rectangles[2].width == 41 &&
            renderer.text_calls.size() == 2 &&
            renderer.text_calls[0].text ==
                std::string("\x81\x40Goblin") &&
            renderer.text_calls[0].draw.x == 77 &&
            renderer.text_calls[0].draw.y == 51 &&
            renderer.text_calls[1].draw.x == 76 &&
            renderer.text_calls[1].draw.y == 50 &&
            renderer.status_icon_calls.size() == 1 &&
            renderer.status_icon_calls[0].pattern == 5 &&
            renderer.status_icon_calls[0].draw.x == 74 &&
            renderer.status_icon_calls[0].draw.y == 51,
        "Enemy hover did not reproduce the retail health bar, "
        "element icon, or name placement.");
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

    appendI32(bytes, 1);
    appendI32(bytes, 7);
    appendI32(bytes, 2);
    appendI32(bytes, 13);
    appendI32(bytes, 8);
    appendI32(bytes, 1);
    appendI32(bytes, 99);

    appendI32(bytes, 1);
    appendCommonEntity(
        bytes, 50, 7, "", 10, 20, 0, false);
    for (std::int32_t value :
         {1, 3, -1, 1, 120, 1, 30000,
          16, 1000, 1, 750, 600, 1500}) {
        appendI32(bytes, value);
    }

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

    appendI32(bytes, 1);
    appendCommonEntity(
        bytes, 6, 99, "Test Enemy", 700, 800, 2, false);
    for (std::int32_t value = 0; value < 15; ++value) {
        appendI32(bytes, value * 3 - 17);
    }
    const std::string ai_control = "Test Chase";
    bytes.insert(
        bytes.end(), ai_control.begin(), ai_control.end());
    bytes.resize(bytes.size() + 32 - ai_control.size(), 0);
    for (std::int32_t value = 0; value < 56; ++value) {
        appendI32(bytes, value * 5 + 11);
    }

    appendI32(bytes, 1);
    appendCommonEntity(
        bytes, 7, -1, "", 900, 1000, 4, false);
    appendI32(bytes, 4);
    appendI32(bytes, 0);
    appendI32(bytes, 25);
    appendI32(bytes, 75);

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
    const osf::ScenarioObject* object =
        scenario.objects().empty()
            ? nullptr
            : &scenario.objects().front();
    const osf::ScenarioEnemy* enemy =
        scenario.enemies().empty()
            ? nullptr
            : &scenario.enemies().front();
    const osf::ScenarioItem* item =
        scenario.items().empty()
            ? nullptr
            : &scenario.items().front();
    return check(
        scenario.controllerPath() ==
                "System\\Game\\Parameter\\Control.aid" &&
            scenario.mapPath() == "Map\\test_01.map" &&
            scenario.title() == "Test Place" &&
            scenario.musicTrack() == 6 &&
            scenario.objectResourceIds() ==
                std::vector<std::int32_t>{7} &&
            scenario.peopleResourceIds() ==
                std::vector<std::int32_t>{13, 8} &&
            scenario.enemyResourceIds() ==
                std::vector<std::int32_t>{99} &&
            scenario.objects().size() == 1 &&
            object &&
            object->id == 50 &&
            object->resource_id == 7 &&
            object->unknown_common_value == 1 &&
            object->visual_mode == 1 &&
            object->static_pattern == 3 &&
            object->animation_chart == -1 &&
            object->draw_status_bit_80 &&
            object->height == 120 &&
            object->unknown_tail_5 == 1 &&
            object->unknown_tail_6 == 30000 &&
            object->draw_flags == 16 &&
            object->draw_strength == 1000 &&
            object->unknown_tail_9 == 1 &&
            object->red_draw_strength == 750 &&
            object->green_draw_strength == 600 &&
            object->blue_draw_strength == 1500 &&
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
            person->unknown_common_value == 1 &&
            person->walk_speed == 10 &&
            person->walk_duration == 30 &&
            person->idle_duration == 30 &&
            person->wander_bounds_relative &&
            person->wander_left == -40 &&
            person->wander_top == -20 &&
            person->wander_right == 60 &&
            person->wander_bottom == 80 &&
            person->wandering_enabled &&
            person->scripted_turning_enabled &&
            person->reserved_behavior_value == -65 &&
            scenario.enemies().size() == 1 &&
            enemy &&
            enemy->id == 6 &&
            enemy->resource_id == 99 &&
            enemy->name == "Test Enemy" &&
            enemy->world_x == 700 &&
            enemy->world_y == 800 &&
            enemy->direction == 2 &&
            enemy->pre_ai_values.front() == -17 &&
            enemy->pre_ai_values.back() == 25 &&
            enemy->ai_control_name == "Test Chase" &&
            enemy->post_ai_values.front() == 11 &&
            enemy->post_ai_values.back() == 286 &&
            enemy->patrol_left ==
                enemy->pre_ai_values[1] &&
            enemy->patrol_top ==
                enemy->pre_ai_values[2] &&
            enemy->patrol_right ==
                enemy->pre_ai_values[3] &&
            enemy->patrol_bottom ==
                enemy->pre_ai_values[4] &&
            enemy->maximum_life ==
                enemy->pre_ai_values[8] &&
            enemy->native_element ==
                enemy->pre_ai_values[6] &&
            enemy->physical_defense ==
                enemy->pre_ai_values[9] &&
            enemy->magical_defense ==
                enemy->pre_ai_values[11] &&
            enemy->magical_evasion ==
                enemy->pre_ai_values[12] &&
            enemy->experience_reward ==
                enemy->pre_ai_values[13] &&
            enemy->loot_table_row ==
                enemy->pre_ai_values[14] &&
            enemy->gold_drop_chance ==
                enemy->post_ai_values[26] &&
            enemy->gold_minimum ==
                enemy->post_ai_values[27] &&
            enemy->gold_maximum ==
                enemy->post_ai_values[28] &&
            enemy->reaction_chance_defense ==
                enemy->post_ai_values[38] &&
            enemy->reaction_duration_defense ==
                enemy->post_ai_values[39] &&
            enemy->always_suppress_reaction_displacement ==
                (enemy->post_ai_values[40] == 1) &&
            enemy->movement_speed_scale ==
                enemy->post_ai_values[54] &&
            enemy->presentation
                    .packet_word_31 ==
                enemy->pre_ai_values[7] &&
            enemy->presentation
                    .native_element ==
                enemy->pre_ai_values[6] &&
            enemy->presentation
                    .direct_special_effect_number ==
                enemy->post_ai_values[21] &&
            enemy->presentation
                    .direct_special_constructor_value_6 ==
                enemy->post_ai_values[23] &&
            enemy->presentation
                    .direct_special_constructor_value_7 ==
                enemy->post_ai_values[22] &&
            enemy->presentation
                    .direct_special_constructor_value_21 ==
                enemy->post_ai_values[24] &&
            enemy->presentation
                    .direct_special_variant ==
                enemy->post_ai_values[25] &&
            enemy->presentation.direct_packet_word_4 ==
                std::array<std::int32_t, 3>{
                    enemy->post_ai_values[0],
                    enemy->post_ai_values[1],
                    enemy->post_ai_values[2]} &&
            enemy->presentation.direct_hit_rate ==
                std::array<std::int32_t, 3>{
                    enemy->post_ai_values[6],
                    enemy->post_ai_values[7],
                    enemy->post_ai_values[8]} &&
            enemy->presentation.direct_packet_word_40 ==
                std::array<std::int32_t, 3>{
                    enemy->post_ai_values[35],
                    enemy->post_ai_values[36],
                    enemy->post_ai_values[37]} &&
            enemy->presentation.direct_packet_word_41 ==
                std::array<std::int32_t, 3>{
                    enemy->post_ai_values[29],
                    enemy->post_ai_values[30],
                    enemy->post_ai_values[31]} &&
            enemy->presentation.direct_packet_word_43 ==
                std::array<std::int32_t, 3>{
                    enemy->post_ai_values[32],
                    enemy->post_ai_values[33],
                    enemy->post_ai_values[34]} &&
            enemy->presentation
                    .direct_maximum_target_distance ==
                std::array<std::int32_t, 3>{
                    enemy->post_ai_values[3],
                    enemy->post_ai_values[4],
                    enemy->post_ai_values[5]} &&
            enemy->presentation.direct_animation_chart ==
                std::array<std::int32_t, 3>{
                    enemy->post_ai_values[41] + 4,
                    enemy->post_ai_values[42] + 4,
                    enemy->post_ai_values[43] + 4} &&
            enemy->presentation
                    .direct_animation_speed_index ==
                std::array<std::int32_t, 3>{
                    enemy->post_ai_values[47],
                    enemy->post_ai_values[48],
                    enemy->post_ai_values[49]} &&
            enemy->presentation.effect_type ==
                std::array<std::int32_t, 3>{
                    enemy->post_ai_values[9],
                    enemy->post_ai_values[10],
                    enemy->post_ai_values[11]} &&
            enemy->presentation.effect_subtype ==
                std::array<std::int32_t, 3>{
                    enemy->post_ai_values[15],
                    enemy->post_ai_values[16],
                    enemy->post_ai_values[17]} &&
            enemy->presentation.effect_parameter ==
                std::array<std::int32_t, 3>{
                    enemy->post_ai_values[12],
                    enemy->post_ai_values[13],
                    enemy->post_ai_values[14]} &&
            enemy->presentation.effect_additive ==
                std::array<std::int32_t, 3>{
                    enemy->post_ai_values[18],
                    enemy->post_ai_values[19],
                    enemy->post_ai_values[20]} &&
            enemy->presentation.effect_animation_chart ==
                std::array<std::int32_t, 3>{
                    enemy->post_ai_values[44] + 7,
                    enemy->post_ai_values[45] + 7,
                    enemy->post_ai_values[46] + 7} &&
            enemy->presentation
                    .effect_animation_speed_index ==
                std::array<std::int32_t, 3>{
                    enemy->post_ai_values[50],
                    enemy->post_ai_values[51],
                    enemy->post_ai_values[52]} &&
            scenario.items().size() == 1 &&
            item &&
            item->id == 7 &&
            item->world_x == 900 &&
            item->world_y == 1000 &&
            item->direction == 4 &&
            item->category == 4 &&
            item->definition_id == 0 &&
            item->minimum_quantity == 25 &&
            item->maximum_quantity == 75 &&
            scenario.entries().size() == 2 &&
            first &&
            first->world_x == 100 &&
            first->world_y == 200 &&
            first->direction == 3 &&
            second &&
            second->world_x == -50 &&
            second->world_y == 60 &&
            second->direction == 7 &&
            scenario.footerValues() ==
                std::array<std::int32_t, 3>{11, 22, 33} &&
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
    if (!check(
            !scenario.decode(bytes),
            "A scenario without its variable data was accepted.")) {
        return false;
    }

    bytes = scenarioFixture();
    bytes.push_back(0xcc);
    if (!check(
            !scenario.decode(bytes),
            "A scenario with bytes after the retail footer was "
            "accepted.")) {
        return false;
    }

    bytes = scenarioFixture();
    writeI32(bytes, bytes.size() - 32, 8);
    return check(
        !scenario.decode(bytes),
        "A scenario entry with a non-retail direction was accepted.");
}

bool testRetailScenarioCatalog() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    const std::filesystem::path scenario_root =
        data_root / "Scenario";
    osf::AiControlDatabase ai_control;
    std::string ai_error;
    if (!check(
            ai_control.load(
                data_root / "System" / "Game" /
                    "Parameter" / "Control.aid",
                &ai_error),
            "The scenario catalog AI-control fixture could not be "
            "decoded.")) {
        std::cerr << ai_error << '\n';
        return false;
    }
    osf::TableDatabase parameter_tables;
    std::string table_error;
    if (!check(
            parameter_tables.load(
                data_root / "System" / "Game" /
                    "Parameter" / "Table.Tbd",
                &table_error),
            "The scenario catalog parameter-table fixture could "
            "not be decoded.")) {
        std::cerr << table_error << '\n';
        return false;
    }
    std::size_t scenario_count = 0;
    std::size_t object_count = 0;
    std::size_t people_count = 0;
    std::size_t enemy_count = 0;
    std::size_t item_count = 0;
    std::set<std::int32_t> people_reserved_values;
    std::set<std::string> enemy_ai_controls;
    std::set<std::int32_t> enemy_resource_ids;
    std::set<std::pair<std::int32_t, std::int32_t>>
        enemy_effect_pairs;
    std::set<std::pair<std::int32_t, std::int32_t>>
        enemy_direct_special_pairs;
    std::map<std::int32_t, std::int32_t>
        enemy_maximum_presentation_chart;
    std::size_t enemy_hole_count = 0;
    bool enemy_holes_match = true;
    bool enemy_hole_runtime_matches = true;
    bool enemy_presentation_values_match = true;
    bool item_common_records_match = true;
    for (const auto& entry :
         std::filesystem::recursive_directory_iterator(
             scenario_root)) {
        if (!entry.is_regular_file() ||
            entry.path().filename() != "Scenario.Mct") {
            continue;
        }
        osf::ScenarioData scenario;
        std::string error;
        if (!check(
                scenario.load(entry.path(), &error),
                "A retail scenario MCT failed the exact sequential "
                "decoder.")) {
            std::cerr << entry.path() << ": " << error << '\n';
            return false;
        }
        for (const osf::ScenarioObject& object :
             scenario.objects()) {
            if (object.initial_state_values.size() != 3) {
                std::cerr
                    << entry.path()
                    << ": object "
                    << object.id
                    << " does not contain three state values.\n";
                return false;
            }
            if (object.resource_id >= 0 &&
                std::find(
                    scenario.objectResourceIds().begin(),
                    scenario.objectResourceIds().end(),
                    object.resource_id) ==
                    scenario.objectResourceIds().end()) {
                std::cerr
                    << entry.path()
                    << ": object resource "
                    << object.resource_id
                    << " is absent from its preload list.\n";
                return false;
            }
        }
        for (const osf::ScenarioPerson& person :
             scenario.people()) {
            people_reserved_values.insert(
                person.reserved_behavior_value);
            if (person.initial_state_values.size() != 3) {
                std::cerr
                    << entry.path()
                    << ": PEOPLE actor "
                    << person.id
                    << " does not contain three state values.\n";
                return false;
            }
            if (person.resource_id >= 0 &&
                std::find(
                    scenario.peopleResourceIds().begin(),
                    scenario.peopleResourceIds().end(),
                    person.resource_id) ==
                    scenario.peopleResourceIds().end()) {
                std::cerr
                    << entry.path()
                    << ": PEOPLE resource "
                    << person.resource_id
                    << " is absent from its preload list.\n";
                return false;
            }
        }
        for (const osf::ScenarioEnemy& enemy :
             scenario.enemies()) {
            enemy_direct_special_pairs.emplace(
                enemy.presentation
                    .direct_special_effect_number,
                enemy.presentation
                    .direct_special_variant);
            for (std::size_t variant = 0;
                 variant < 3;
                 ++variant) {
                enemy_effect_pairs.emplace(
                    enemy.presentation.effect_type[variant],
                    enemy.presentation
                        .effect_subtype[variant]);
            }
            enemy_ai_controls.insert(enemy.ai_control_name);
            const osf::AiControlList* control =
                ai_control.find(enemy.ai_control_name);
            const std::int32_t control_index =
                ai_control.indexOf(control);
            if (!control || control_index < 0) {
                std::cerr
                    << entry.path()
                    << ": enemy " << enemy.id
                    << " references an unknown AI-control list.\n";
                return false;
            }
            if (enemy.resource_id >= 0) {
                enemy_resource_ids.insert(enemy.resource_id);
                const auto speed_valid =
                    [](std::int32_t speed_index) {
                        return speed_index >= 0 &&
                               speed_index < 10;
                    };
                enemy_presentation_values_match =
                    enemy_presentation_values_match &&
                    std::all_of(
                        enemy.presentation
                            .direct_animation_speed_index
                            .begin(),
                        enemy.presentation
                            .direct_animation_speed_index
                            .end(),
                        speed_valid) &&
                    std::all_of(
                        enemy.presentation
                            .effect_animation_speed_index
                            .begin(),
                        enemy.presentation
                            .effect_animation_speed_index
                            .end(),
                        speed_valid);
                std::int32_t& maximum_chart =
                    enemy_maximum_presentation_chart[
                        enemy.resource_id];
                maximum_chart = std::max(
                    maximum_chart,
                    *std::max_element(
                        enemy.presentation
                            .direct_animation_chart
                            .begin(),
                        enemy.presentation
                            .direct_animation_chart
                            .end()));
                maximum_chart = std::max(
                    maximum_chart,
                    *std::max_element(
                        enemy.presentation
                            .effect_animation_chart
                            .begin(),
                        enemy.presentation
                            .effect_animation_chart
                            .end()));
            } else {
                ++enemy_hole_count;
                enemy_holes_match =
                    enemy_holes_match &&
                    enemy.resource_id == -1 &&
                    enemy.name == "Enemy Hole" &&
                    enemy.initial_state_values ==
                        std::vector<std::int32_t>{0, 1, 0};
                osf::EnemyActor enemy_hole;
                std::string actor_error;
                enemy_hole_runtime_matches =
                    enemy_hole_runtime_matches &&
                    enemy_hole.initialize(
                        enemy,
                        nullptr,
                        *control,
                        control_index,
                        &actor_error) &&
                    !enemy_hole.hasVisual() &&
                    !enemy_hole.visible() &&
                    enemy_hole.pointerEnabled() &&
                    !enemy_hole.judgementEnabled() &&
                    actor_error.empty();
            }
            if (enemy.initial_state_values.size() != 3) {
                std::cerr
                    << entry.path()
                    << ": enemy "
                    << enemy.id
                    << " does not contain three state values.\n";
                return false;
            }
            if (enemy.resource_id >= 0 &&
                std::find(
                    scenario.enemyResourceIds().begin(),
                    scenario.enemyResourceIds().end(),
                    enemy.resource_id) ==
                    scenario.enemyResourceIds().end()) {
                std::cerr
                    << entry.path()
                    << ": enemy resource "
                    << enemy.resource_id
                    << " is absent from its preload list.\n";
                return false;
            }
        }
        for (const osf::ScenarioItem& item :
             scenario.items()) {
            if (item.initial_state_values.size() != 3 ||
                item.minimum_quantity > item.maximum_quantity) {
                std::cerr
                    << entry.path()
                    << ": placed item "
                    << item.id
                    << " has an invalid state or quantity range.\n";
                return false;
            }
            item_common_records_match =
                item_common_records_match &&
                item.initial_state_values ==
                    std::vector<std::int32_t>{1, 1, 1} &&
                item.resource_id == 0 &&
                item.name.empty() &&
                item.judgement_left == -40 &&
                item.judgement_top == -40 &&
                item.judgement_right == 39 &&
                item.judgement_bottom == 39 &&
                item.direction == 8 &&
                item.minimum_quantity == 0 &&
                item.maximum_quantity == 0;
        }
        ++scenario_count;
        object_count += scenario.objects().size();
        people_count += scenario.people().size();
        enemy_count += scenario.enemies().size();
        item_count += scenario.items().size();
    }
    osf::CharacterVisualResources enemy_visuals{"ENEMY"};
    for (const auto& effect : enemy_effect_pairs) {
        if (effect.first == -1) {
            continue;
        }
        osf::RetailRandom random(1);
        osf::EnemyEffectImpactInput input;
        input.type = effect.first;
        input.subtype = effect.second;
        const osf::CombatEffectSpawnRequest request =
            osf::resolveEnemyEffectImpact(
                input, parameter_tables, random);
        bool tables_contain_pair =
            parameter_tables.find(18) &&
            parameter_tables.find(18)->contains(
                effect.first, effect.second - 1) &&
            parameter_tables.find(19) &&
            parameter_tables.find(19)->contains(
                effect.first, 0) &&
            parameter_tables.find(21) &&
            parameter_tables.find(21)->contains(
                effect.first, effect.second - 1) &&
            parameter_tables.find(35) &&
            parameter_tables.find(35)->contains(
                effect.first, effect.second - 1);
        for (std::int32_t table_number = 70;
             table_number <= 78;
             ++table_number) {
            const osf::TableData* table =
                parameter_tables.find(table_number);
            tables_contain_pair =
                tables_contain_pair &&
                table &&
                table->contains(
                    effect.first,
                    effect.second * 3 - 1);
        }
        if (!check(
                request.valid &&
                    tables_contain_pair,
                "A shipped enemy effect type/subtype pair is "
                "not covered by its retail constructor or "
                "parameter tables.")) {
            std::cerr
                << "effect type/subtype: "
                << effect.first << '/'
                << effect.second << '\n';
            return false;
        }
    }
    for (const std::int32_t resource_id :
         enemy_resource_ids) {
        std::string error;
        const osf::CharacterVisualResource* visual =
            enemy_visuals.load(
                data_root, resource_id, &error);
        if (!check(
                visual &&
                    !visual->animation().charts().empty() &&
                    enemy_maximum_presentation_chart[
                        resource_id] >= 0 &&
                    static_cast<std::size_t>(
                        enemy_maximum_presentation_chart[
                            resource_id]) <
                        visual->animation().charts().size(),
                "A shipped MCT enemy resource could not be decoded.")) {
            std::cerr
                << "ENEMY resource " << resource_id
                << ": " << error << '\n';
            return false;
        }
    }
    if (scenario_count != 209 ||
        object_count != 5203 ||
        people_count != 163 ||
        enemy_count != 18788 ||
        item_count != 84) {
        std::cerr
            << "catalog counts: scenarios=" << scenario_count
            << ", objects=" << object_count
            << ", people=" << people_count
            << ", enemies=" << enemy_count
            << ", items=" << item_count << '\n';
    }
    return check(
        scenario_count == 209 &&
            object_count == 5203 &&
            people_count == 163 &&
            enemy_count == 18788 &&
            item_count == 84 &&
            item_common_records_match &&
            enemy_hole_count == 34 &&
            enemy_holes_match &&
            enemy_hole_runtime_matches &&
            enemy_presentation_values_match &&
            people_reserved_values ==
                std::set<std::int32_t>{-100, -85, -65} &&
            enemy_direct_special_pairs ==
                std::set<
                    std::pair<
                        std::int32_t,
                        std::int32_t>>{
                    {-1, 0},
                    {0, 0},
                    {1, 0},
                    {4, 0},
                    {5, 0},
                    {6, 1},
                    {7, 0},
                } &&
            !enemy_ai_controls.empty() &&
            enemy_ai_controls.find("") ==
                enemy_ai_controls.end(),
        "The retail MCT catalog counts differ from the exact loader "
        "trace.");
#else
    return true;
#endif
}

bool testWorldItemSaveRoundTrip() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    std::string error;
    osf::WorldScene saved_world;
    osf::PlayerLoadRequest new_player;
    new_player.name = "ItemSave";
    if (!check(
            saved_world.loadInitialScenario(
                data_root,
                new_player,
                &error),
            "The item save fixture world could not be loaded.")) {
        return false;
    }

    const std::filesystem::path save_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_world_item_save_test";
    const std::filesystem::path save_path =
        save_root / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(save_root, cleanup_error);
    const osf::ItemDefinition* special_gold =
        saved_world.itemDatabase().find(4, 0);
    const osf::ItemDefinition* spirit_stone =
        saved_world.itemDatabase().find(4, 98000001);
    if (!check(
            special_gold && spirit_stone &&
                saved_world.playerSpecialItems()
                    .place(
                        osf::makeInventoryItem(
                            *special_gold, 125),
                        2,
                        3)
                    .accepted &&
                saved_world.playerAutomaticItems().add(
                    *spirit_stone,
                    osf::makeInventoryItem(*spirit_stone)),
            "The special-item save fixture could not be placed.")) {
        return false;
    }
    osf::PlayerGiantWarehouse::EnabledFlags giant_flags{};
    giant_flags[0] = 1;
    giant_flags[2] = 1;
    saved_world.playerGiantWarehouse().restoreEnabledFlags(
        giant_flags);
    if (!check(
            saved_world.playerGiantWarehouse()
                .page(2)
                .place(
                    osf::makeInventoryItem(*special_gold, 50),
                    4,
                    5)
                .accepted,
            "The Giant Warehouse save fixture could not be placed.")) {
        return false;
    }
    if (!check(
            saved_world.transitionScenario({6, 4, 0}, &error) ==
                    osf::ScenarioTravelResult::loaded &&
                saved_world.scenarioId() == 6 &&
                saved_world.retailSaveWorldState().entry_value == 4 &&
                saved_world.playerWorldX() == 35105 &&
                saved_world.playerWorldY() == -6156,
            "The save-location fixture could not enter Wasteland of "
            "Pillars through retail entry four.")) {
        std::cerr << error << '\n';
        return false;
    }
    if (!check(
            osf::writeRetailSave(
                save_path,
                saved_world.playerData(),
                saved_world.itemDatabase(),
                saved_world.playerInventory(),
                saved_world.playerEquipment(),
                saved_world.playerBelt(),
                saved_world.playerSpecialItems(),
                saved_world.retailSaveProgress(),
                saved_world.playerMagic(),
                saved_world.playerMineCount(),
                saved_world.retailSaveWorldState(),
                saved_world.playerGiantWarehouse(),
                saved_world.playerAutomaticItems(),
                0x5a,
                &error),
            "The world-owned item state could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerLoadRequest saved_player;
    saved_player.source = osf::PlayerDataSource::retail_save;
    saved_player.save_path = save_path;
    osf::WorldScene loaded_world;
    const bool loaded =
        loaded_world.loadInitialScenario(
            data_root,
            saved_player,
            &error);
    std::filesystem::remove_all(save_root, cleanup_error);
    if (!check(
            loaded &&
                loaded_world.scenarioId() == 6 &&
                loaded_world.retailSaveWorldState().entry_value == 4 &&
                loaded_world.playerWorldX() == 35105 &&
                loaded_world.playerWorldY() == -6156 &&
                loaded_world.playerInventory().items().size() ==
                    saved_world.playerInventory().items().size() &&
                loaded_world.playerBelt().items().size() ==
                    saved_world.playerBelt().items().size() &&
                loaded_world.playerInventory().itemAt(0, 0) &&
                loaded_world.playerInventory()
                        .itemAt(0, 0)
                        ->definition_id == 0 &&
                loaded_world.playerInventory().itemAt(1, 3) &&
                loaded_world.playerInventory()
                        .itemAt(1, 3)
                        ->definition_id == 10000000 &&
                loaded_world.playerBelt().itemAt(3, 0) &&
                loaded_world.playerBelt()
                        .itemAt(3, 0)
                        ->definition_id == 0 &&
                loaded_world.playerBelt().itemAt(3, 1) &&
                loaded_world.playerBelt()
                        .itemAt(3, 1)
                        ->definition_id == 10000000 &&
                loaded_world.playerEquipment().item(
                    osf::EquipmentSlot::body) &&
                loaded_world.playerEquipment()
                        .item(osf::EquipmentSlot::body)
                        ->category == 1 &&
                loaded_world.playerEquipment()
                        .item(osf::EquipmentSlot::body)
                        ->definition_id == 0 &&
                loaded_world.playerSpecialItems().items().size() == 1 &&
                loaded_world.playerSpecialItems().items()[0].category == 4 &&
                loaded_world.playerSpecialItems().items()[0].quantity == 125 &&
                loaded_world.playerSpecialItems().items()[0].grid_x == 2 &&
                loaded_world.playerSpecialItems().items()[0].grid_y == 3 &&
                loaded_world.playerGiantWarehouse().pageEnabled(2) &&
                loaded_world.playerGiantWarehouse()
                        .page(2)
                        .items()
                        .size() == 1 &&
                loaded_world.playerGiantWarehouse()
                        .page(2)
                        .items()[0]
                        .quantity == 50 &&
                loaded_world.playerGiantWarehouse()
                        .page(2)
                        .items()[0]
                        .grid_x == 4 &&
                loaded_world.playerGiantWarehouse()
                        .page(2)
                        .items()[0]
                        .grid_y == 5 &&
                loaded_world.playerAutomaticItems().contains(
                    4, 98000001) &&
                loaded_world.playerAutomaticItems()
                        .page(2)
                        .items()[0]
                        .grid_x == 1 &&
                loaded_world.playerAutomaticItems()
                        .page(2)
                        .items()[0]
                        .grid_y == 0,
            "World loading discarded backpack, belt, equipped, Warehouse, "
            "or automatic items.")) {
        std::cerr << error << '\n';
        return false;
    }
    return true;
#else
    return true;
#endif
}

bool testPersistentConversationAndMovementState() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    std::string error;
    osf::WorldScene world;
    osf::PlayerLoadRequest player;
    player.name = "Progress";
    if (!check(
            world.loadInitialScenario(
                data_root, player, &error),
            "The persistent-state fixture world could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::ScreenPosition ostare_pointer;
    if (!check(
            findNpcPointerPoint(world, 0, ostare_pointer) &&
                world.commandWorldInteraction(
                    ostare_pointer.x, ostare_pointer.y) &&
                updateUntilConversation(world) &&
                world.conversationMessageId() == 1000000,
            "Ostare's first conversation could not be prepared for saving.")) {
        return false;
    }
    for (std::int32_t message = 0; message < 5; ++message) {
        world.advanceConversation();
    }
    world.togglePlayerRun();
    const osf::RetailSaveProgress live_progress =
        world.retailSaveProgress();
    if (!check(
            !world.conversationActive() &&
                world.quests().state(4) == 0 &&
                live_progress.script_state_flags.size() > 4 &&
                live_progress.script_state_flags[4] == 1 &&
                world.playerMovementPace() ==
                    osf::MovementPace::run,
            "The conversation or movement state was not live before saving.")) {
        return false;
    }

    const std::filesystem::path save_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_progress_save_test";
    const std::filesystem::path save_path =
        save_root / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(save_root, cleanup_error);
    if (!check(
            osf::writeRetailSave(
                save_path,
                world.playerData(),
                world.itemDatabase(),
                world.playerInventory(),
                world.playerEquipment(),
                world.playerBelt(),
                world.playerSpecialItems(),
                world.retailSaveProgress(),
                world.playerMagic(),
                world.playerMineCount(),
                world.retailSaveWorldState(),
                world.playerGiantWarehouse(),
                world.playerAutomaticItems(),
                0x6d,
                &error),
            "The conversation and movement state could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerLoadRequest saved_player;
    saved_player.source = osf::PlayerDataSource::retail_save;
    saved_player.save_path = save_path;
    osf::WorldScene restored;
    const bool loaded =
        restored.loadInitialScenario(
            data_root, saved_player, &error);
    const osf::RetailSaveProgress restored_progress =
        restored.retailSaveProgress();
    const bool restored_state =
        loaded &&
        restored.quests().state(4) == 0 &&
        restored_progress.script_state_flags.size() > 4 &&
        restored_progress.script_state_flags[4] == 1 &&
        restored.playerMovementPace() ==
            osf::MovementPace::run &&
        restored.groundItems().empty() &&
        findNpcPointerPoint(restored, 0, ostare_pointer) &&
        restored.commandWorldInteraction(
            ostare_pointer.x, ostare_pointer.y) &&
        updateUntilConversation(restored) &&
        restored.conversationMessageId() == 1000005 &&
        restored.groundItems().empty();
    std::filesystem::remove_all(save_root, cleanup_error);
    if (!check(
            restored_state,
            "Saved quest/conversation state repeated Ostare's starter "
            "drop or lost the run/walk choice.")) {
        std::cerr << error << '\n';
        return false;
    }
    return true;
#else
    return true;
#endif
}

bool testLegacyDeadSaveRecovery() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    std::string error;
    osf::TableDatabase tables;
    osf::ItemDatabase items;
    osf::PlayerData dead_player;
    if (!check(
            tables.load(
                data_root / "System" / "Game" /
                    "Parameter" / "Table.Tbd",
                &error) &&
                items.load(
                    data_root / "System" / "Game" /
                        "Parameter" / "Item.Ibn",
                    &error) &&
                dead_player.initializeNew(
                    "DeadSave", 0, tables, &error),
            "The dead-save recovery fixture could not be prepared.")) {
        std::cerr << error << '\n';
        return false;
    }
    dead_player.setCurrentLife(0);
    dead_player.setCurrentMana(1);

    const std::filesystem::path save_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_dead_save_recovery_test";
    const std::filesystem::path save_path =
        save_root / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(save_root, cleanup_error);
    const osf::PlayerInventory inventory;
    const osf::PlayerEquipment equipment;
    const osf::PlayerBelt belt;
    const osf::PlayerSpecialItems special_items;
    if (!check(
            osf::writeRetailSave(
                save_path,
                dead_player,
                items,
                inventory,
                equipment,
                belt,
                special_items,
                0x52,
                &error),
            "The legacy zero-life save fixture could not be written.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerLoadRequest request;
    request.source = osf::PlayerDataSource::retail_save;
    request.save_path = save_path;
    osf::WorldScene world;
    const bool loaded =
        world.loadInitialScenario(data_root, request, &error);
    std::filesystem::remove_all(save_root, cleanup_error);
    return check(
        loaded &&
            world.playerData().currentLife() ==
                world.playerData().baseMaximumLife() &&
            world.playerData().currentMana() ==
                world.playerData().baseMaximumMana() &&
            world.playerMotion() == osf::PlayerMotion::idle,
        "A save made by the old portable dead-state bug did not "
        "enter town through the retail revive reset.");
#else
    return true;
#endif
}

bool testGeneralScenarioStart() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    osf::PlayerLoadRequest player;
    player.name = "Traveler";
    std::string error;
    osf::WorldScene wasteland;
    if (!check(
            wasteland.loadInitialScenario(
                data_root,
                player,
                {6, 4, 0},
                &error),
            "Wasteland of Pillars could not be loaded through the "
            "general scenario start path.")) {
        std::cerr << error << '\n';
        return false;
    }
    const osf::ScenarioEntry* entry =
        wasteland.scenario().findEntry(16);
    if (!check(
            wasteland.scenarioId() == 6 &&
                wasteland.scenario().title() ==
                    "Wasteland of Pillars" &&
                wasteland.scenario().mapPath() ==
                    "Map\\f00_07.map" &&
                wasteland.musicTrack() == 1 &&
                entry &&
                entry->world_x == 35105 &&
                entry->world_y == -6156 &&
                entry->direction == 7 &&
                wasteland.playerWorldX() == entry->world_x &&
                wasteland.playerWorldY() == entry->world_y &&
                wasteland.playerDirection() == entry->direction &&
                wasteland.scenarioObjects().size() == 35 &&
                wasteland.npcs().size() == 2 &&
                wasteland.enemies().size() == 66 &&
                wasteland.ground().width() > 0 &&
                wasteland.ground().height() > 0 &&
                !wasteland.objectMap().objects().empty() &&
                !wasteland.mapPatterns().empty() &&
                !wasteland.mapOverviewPatterns()
                     .patterns()
                     .empty(),
            "The decimal scenario directory, entry key, map, actors, "
            "or music differ from retail scenario 6.")) {
        return false;
    }

    const osf::EnemyActor& first_enemy =
        wasteland.enemies().front();
    if (!check(
            first_enemy.id() == 0 &&
                first_enemy.characterNumber() == 14000000 &&
                first_enemy.resourceId() == 1 &&
                first_enemy.name() == "Spike Centinel" &&
                first_enemy.labelHeight() == 70 &&
                first_enemy.position().x == 27480 &&
                first_enemy.position().y == -10341 &&
                first_enemy.judgement().left == -80 &&
                first_enemy.judgement().top == -80 &&
                first_enemy.judgement().right == 79 &&
                first_enemy.judgement().bottom == 79 &&
                first_enemy.direction() == 7 &&
                first_enemy.animationChart() == 0 &&
                first_enemy.animationFrame() == 0 &&
                !first_enemy.aiControlName().empty() &&
                first_enemy.aiControl() &&
                first_enemy.aiControl()->name() ==
                    first_enemy.aiControlName() &&
                first_enemy.aiControlIndex() >= 0 &&
                wasteland.aiControlDatabase().indexOf(
                    first_enemy.aiControl()) ==
                    first_enemy.aiControlIndex() &&
                first_enemy.patrolBounds().left == -800 &&
                first_enemy.patrolBounds().top == -800 &&
                first_enemy.patrolBounds().right == 799 &&
                first_enemy.patrolBounds().bottom == 799 &&
                first_enemy.currentLife() == 400 &&
                first_enemy.maximumLife() == 400 &&
                first_enemy.nativeElement() == 0 &&
                first_enemy.physicalDefense() == 20 &&
                first_enemy.magicalDefense() == 25 &&
                first_enemy.magicalEvasion() ==
                    wasteland.scenario()
                        .enemies()
                        .front()
                        .pre_ai_values[12] &&
                first_enemy.reactionChanceDefense() == 0 &&
                first_enemy.reactionDurationDefense() == 0 &&
                !first_enemy
                     .alwaysSuppressReactionDisplacement() &&
                first_enemy.movementSpeedScale() == 3000 &&
                first_enemy.visible() &&
                first_enemy.pointerEnabled() &&
                first_enemy.judgementEnabled() &&
                !first_enemy.animation().charts().empty(),
            "The first Wasteland enemy did not preserve its common "
            "MCT identity, visual, state, or idle action.")) {
        return false;
    }

    NpcRecordingBackend enemy_renderer;
    enemy_renderer.patterns =
        &first_enemy.patterns();
    enemy_renderer.shadows =
        &first_enemy.shadowPatterns();
    osf::renderWorldGeometry(
        enemy_renderer, wasteland);
    if (!check(
            !enemy_renderer.calls.empty() &&
                std::any_of(
                    enemy_renderer.calls.begin(),
                    enemy_renderer.calls.end(),
                    [](const NpcPatternCall& call) {
                        return call.shadow;
                    }) &&
                std::any_of(
                    enemy_renderer.calls.begin(),
                    enemy_renderer.calls.end(),
                    [](const NpcPatternCall& call) {
                        return !call.shadow;
                    }),
            "Wasteland enemies did not join both retail world render "
            "passes.")) {
        return false;
    }
    const osf::WorldPosition enemy_position =
        first_enemy.position();
    wasteland.update();
    if (!check(
            wasteland.enemies().front().animationChart() == 0 &&
                wasteland.enemies().front().animationFrame() == 0 &&
                wasteland.enemies().front().position().x ==
                    enemy_position.x &&
                wasteland.enemies().front().position().y ==
                    enemy_position.y,
            "An enemy outside the retail activation range began "
            "its authored patrol.")) {
        return false;
    }
    wasteland.update();
    if (!check(
            wasteland.enemies().front().animationChart() == 0 &&
                wasteland.enemies().front().position().x ==
                    enemy_position.x &&
                wasteland.enemies().front().position().y ==
                    enemy_position.y,
            "The inactive enemy did not remain idle on the shared "
            "active-map cadence.")) {
        return false;
    }
    const std::int32_t life_before_enemy_ai =
        wasteland.playerData().currentLife();
    const std::size_t inventory_before_enemy_ai =
        wasteland.playerInventory().items().size();
    const std::size_t belt_before_enemy_ai =
        wasteland.playerBelt().items().size();
    bool heard_enemy_hit = false;
    bool saw_player_hit_splatter = false;
    for (std::int32_t update = 0;
         update < 300 &&
         wasteland.playerData().currentLife() ==
             life_before_enemy_ai;
         ++update) {
        wasteland.update();
        const std::vector<std::int32_t> samples =
            wasteland.takeAudioSamples();
        heard_enemy_hit =
            heard_enemy_hit ||
            std::find(
                samples.begin(), samples.end(), 6) !=
                samples.end();
        saw_player_hit_splatter =
            saw_player_hit_splatter ||
            std::any_of(
                wasteland.combatEffects().begin(),
                wasteland.combatEffects().end(),
                [](const osf::CombatEffectActor& effect) {
                    return effect.effectNumber() >= 21000 &&
                           effect.effectNumber() <= 21003;
                });
    }
    if (!check(
            wasteland.playerData().currentLife() <
                    life_before_enemy_ai &&
                heard_enemy_hit &&
                saw_player_hit_splatter &&
                wasteland.playerInventory().items().size() ==
                    inventory_before_enemy_ai &&
                wasteland.playerBelt().items().size() ==
                    belt_before_enemy_ai &&
                wasteland.playerEquipment().item(
                    osf::EquipmentSlot::body) &&
                (wasteland.playerMotion() ==
                     osf::PlayerMotion::reacting ||
                 wasteland.playerMotion() ==
                     osf::PlayerMotion::defeated),
            "A live Wasteland enemy did not approach, complete its "
            "authored attack presentation, pass damage through the "
            "player receiver, and publish its impact splatter and "
            "sample.")) {
        return false;
    }
    const std::int32_t damaged_life =
        wasteland.playerData().currentLife();
    const std::filesystem::path combat_save_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_enemy_ai_save_test";
    const std::filesystem::path combat_save =
        combat_save_root / "Save" / "0000.Ssv";
    std::error_code combat_cleanup_error;
    std::filesystem::remove_all(
        combat_save_root, combat_cleanup_error);
    if (!check(
            osf::writeRetailSave(
                combat_save,
                wasteland.playerData(),
                wasteland.itemDatabase(),
                wasteland.playerInventory(),
                wasteland.playerEquipment(),
                wasteland.playerBelt(),
                wasteland.playerSpecialItems(),
                0x73,
                &error),
            "The live enemy-hit state could not be written through "
            "the retail save path.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerLoadRequest damaged_player;
    damaged_player.source =
        osf::PlayerDataSource::retail_save;
    damaged_player.save_path = combat_save;
    osf::WorldScene restored_combat;
    const bool combat_loaded =
        restored_combat.loadInitialScenario(
            data_root, damaged_player, &error);
    std::filesystem::remove_all(
        combat_save_root, combat_cleanup_error);
    if (!check(
            combat_loaded &&
                restored_combat.playerData().currentLife() ==
                    damaged_life &&
                restored_combat.playerInventory().items().size() ==
                    inventory_before_enemy_ai &&
                restored_combat.playerBelt().items().size() ==
                    belt_before_enemy_ai &&
                restored_combat.playerEquipment().item(
                    osf::EquipmentSlot::body),
            "Saving after a live enemy hit discarded life or owned "
            "item state on reload.")) {
        std::cerr << error << '\n';
        return false;
    }

    for (const osf::TransportDestination& destination :
         wasteland.transports().destinations()) {
        char directory[16]{};
        std::snprintf(
            directory,
            sizeof(directory),
            "%08d",
            destination.scenario);
        osf::ScenarioData destination_scenario;
        if (!check(
                destination_scenario.load(
                    data_root / "Scenario" / directory /
                        "Scenario.Mct",
                    &error) &&
                    destination_scenario.findEntry(
                        destination.entry * 4),
                "A Table 40 destination does not resolve its single-player "
                "scenario entry.")) {
            std::cerr
                << "Transport row "
                << destination.row << " ("
                << destination.name << "): "
                << error << '\n';
            return false;
        }
    }

    osf::WorldScene invalid_entry;
    error.clear();
    if (!check(
            !invalid_entry.loadInitialScenario(
                data_root,
                player,
                {6, 999, 0},
                &error) &&
                !invalid_entry.hasPlayer() &&
                invalid_entry.scenarioId() == -1 &&
                invalid_entry.scenarioObjects().empty() &&
                error.find("entry key 3996") !=
                    std::string::npos,
            "A missing scenario entry did not fail without leaving a "
            "partially loaded world.")) {
        return false;
    }

    osf::WorldScene missing_scenario;
    error.clear();
    if (!check(
            !missing_scenario.loadInitialScenario(
                data_root,
                player,
                {2, 0, 0},
                &error) &&
                !missing_scenario.hasPlayer() &&
                missing_scenario.scenarioId() == -1 &&
                missing_scenario.scenarioObjects().empty() &&
                !error.empty(),
            "A missing decimal scenario directory left a partially loaded "
            "world.")) {
        return false;
    }

    osf::WorldScene invalid_request;
    error.clear();
    return check(
        !invalid_request.loadInitialScenario(
            data_root,
            player,
            {-1, 0, 0},
            &error) &&
            !invalid_request.hasPlayer() &&
            invalid_request.scenarioId() == -1 &&
            error == "The scenario start request is invalid.",
        "An invalid scenario start request was not rejected before loading.");
#else
    return true;
#endif
}

bool testLiveScenarioTransition() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    osf::WorldScene world;
    osf::PlayerLoadRequest player;
    player.name = "Traveler";
    std::string error;
    if (!check(
            world.loadInitialScenario(
                data_root, player, &error),
            "The live transition fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }

    const osf::ItemDefinition* gold =
        world.itemDatabase().find(4, 0);
    if (!check(
            gold &&
                world.playerSpecialItems()
                    .place(
                        osf::makeInventoryItem(*gold, 125),
                        2,
                        3)
                    .accepted,
            "The transition fixture could not seed Special Items.")) {
        return false;
    }
    const std::size_t inventory_count =
        world.playerInventory().items().size();
    const std::size_t belt_count =
        world.playerBelt().items().size();
    const std::int32_t original_x = world.playerWorldX();
    const std::int32_t original_y = world.playerWorldY();
    const std::size_t original_script_statuses =
        world.scenarioScript().statuses().size();

    error.clear();
    if (!check(
            world.transitionScenario({6, 999, 0}, &error) ==
                    osf::ScenarioTravelResult::failed &&
                world.scenarioId() == 0 &&
                world.playerWorldX() == original_x &&
                world.playerWorldY() == original_y &&
                world.scenarioScript().statuses().size() ==
                    original_script_statuses &&
                world.playerInventory().items().size() ==
                    inventory_count &&
                world.playerBelt().items().size() ==
                    belt_count &&
                world.playerSpecialItems().items().size() == 1 &&
                !error.empty(),
            "A failed cross-map load mutated the live world or a "
            "persistent owner.")) {
        return false;
    }

    if (!check(
            world.transitionScenario({6, 4, 0}, &error) ==
                    osf::ScenarioTravelResult::loaded &&
                world.scenarioId() == 6 &&
                world.musicTrack() == 1 &&
                world.playerWorldX() == 35105 &&
                world.playerWorldY() == -6156 &&
                world.playerDirection() == 7 &&
                world.scenarioObjects().size() == 35 &&
                world.npcs().size() == 2 &&
                world.enemies().size() == 66 &&
                world.groundItems().empty() &&
                world.playerInventory().items().size() ==
                    inventory_count &&
                world.playerBelt().items().size() ==
                    belt_count &&
                world.playerEquipment().item(
                    osf::EquipmentSlot::body) &&
                world.playerEquipment()
                        .item(osf::EquipmentSlot::body)
                        ->definition_id == 0 &&
                world.playerSpecialItems().items().size() == 1 &&
                world.playerSpecialItems().items()[0].quantity == 125 &&
                world.transports().enabled(0) &&
                !world.transports().enabled(1) &&
                world.missions().missions().size() == 48 &&
                world.quests().state(0) == 0,
            "A successful cross-map load lost player-owned state or "
            "progress catalogs, or kept old scenario-local state.")) {
        std::cerr << error << '\n';
        return false;
    }

    world.update();
    if (!check(
            world.transports().enabled(1) &&
                world.retailSaveProgress().transport_flags[1] == 1,
            "Standing on the Wasteland transport did not unlock its "
            "Table 40 destination.")) {
        return false;
    }

    const std::filesystem::path transport_save_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_transport_unlock_save_test";
    const std::filesystem::path transport_save =
        transport_save_root / "Save" / "0000.Ssv";
    std::error_code transport_cleanup_error;
    std::filesystem::remove_all(
        transport_save_root, transport_cleanup_error);
    if (!check(
            osf::writeRetailSave(
                transport_save,
                world.playerData(),
                world.itemDatabase(),
                world.playerInventory(),
                world.playerEquipment(),
                world.playerBelt(),
                world.playerSpecialItems(),
                world.retailSaveProgress(),
                world.playerMagic(),
                0x47,
                &error),
            "The unlocked transport state could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerLoadRequest transport_player;
    transport_player.source =
        osf::PlayerDataSource::retail_save;
    transport_player.save_path = transport_save;
    osf::WorldScene restored_transport;
    const bool restored_transport_loaded =
        restored_transport.loadInitialScenario(
            data_root, transport_player, &error);
    std::filesystem::remove_all(
        transport_save_root, transport_cleanup_error);
    if (!check(
            restored_transport_loaded &&
                restored_transport.transports().enabled(1),
            "The Wasteland transport unlock did not survive a save and "
            "load round trip.")) {
        std::cerr << error << '\n';
        return false;
    }

    const osf::WorldPosition first_enemy_position =
        world.enemies().front().position();
    const std::int32_t first_enemy_character =
        world.enemies().front().characterNumber();
    const osf::AiControlList* first_enemy_control =
        world.enemies().front().aiControl();
    error.clear();
    if (!check(
            world.transitionScenario({2, 0, 0}, &error) ==
                    osf::ScenarioTravelResult::failed &&
                world.scenarioId() == 6 &&
                world.enemies().size() == 66 &&
                world.enemies().front().characterNumber() ==
                    first_enemy_character &&
                world.enemies().front().aiControl() ==
                    first_enemy_control &&
                world.aiControlDatabase().indexOf(
                    first_enemy_control) ==
                    world.enemies().front().aiControlIndex() &&
                world.enemies().front().position().x ==
                    first_enemy_position.x &&
                world.enemies().front().position().y ==
                    first_enemy_position.y &&
                world.playerInventory().items().size() ==
                    inventory_count &&
                world.playerSpecialItems().items().size() == 1 &&
                !error.empty(),
            "A failed map load discarded live enemies or persistent "
            "player ownership.")) {
        return false;
    }

    world.commandPlayerMovement(400, 200);
    world.update();
    return check(
        world.transitionScenario({6, 4, 0}, &error) ==
                osf::ScenarioTravelResult::relocated &&
            world.scenarioId() == 6 &&
            world.playerWorldX() == 35105 &&
            world.playerWorldY() == -6156 &&
            world.playerDirection() == 7 &&
            world.enemies().size() == 66 &&
            world.enemies().front().aiControl() ==
                first_enemy_control &&
            world.playerInventory().items().size() ==
                inventory_count &&
            world.playerSpecialItems().items().size() == 1,
        "The same-map fast path reloaded the scenario or lost ownership.");
#else
    return true;
#endif
}

bool testRetailScenarioEntryInitialization() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    osf::PlayerLoadRequest player;
    player.name = "Traveler";
    std::string error;
    osf::WorldScene world;
    if (!check(
            world.loadInitialScenario(
                data_root, player, {3, 0, 0}, &error) &&
                world.scenarioCaptionMessageId() == 1000000 &&
                world.scenarioCaptionText() ==
                    "Wasteland of Hesitation\n",
            "Opcode 49 did not retain the first Episode 1 outdoor "
            "caption during initial status-kind-seven setup.")) {
        std::cerr << error << '\n';
        return false;
    }
    if (!check(
            world.transitionScenario({10000, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::loaded &&
                world.scenarioId() == 10000 &&
                world.scenarioCaptionMessageId() == 1000000 &&
                world.scenarioCaptionText() ==
                    "Dusty Ruins, B1F\n",
            "The changed-map status initialization did not branch on "
            "the retail entry value.")) {
        std::cerr << error << '\n';
        return false;
    }
    return check(
        world.transitionScenario({10000, 2, 0}, &error) ==
                osf::ScenarioTravelResult::relocated &&
            world.scenarioId() == 10000 &&
            world.scenarioCaptionMessageId() == 1000001 &&
            world.scenarioCaptionText() ==
                "Dusty Ruins, B2F\n",
        "A same-map relocation did not rerun retail status kind seven "
        "with the new entry value.");
#else
    return true;
#endif
}

bool testRetailScenarioObjectOverride() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    osf::PlayerLoadRequest player;
    player.name = "Object Override";
    std::string error;
    osf::WorldScene world;
    if (!check(
            world.loadInitialScenario(
                data_root, player, {1, 0, 0}, &error),
            "The Near Remote Town object-override fixture could not "
            "load.")) {
        std::cerr << error << '\n';
        return false;
    }
    const auto findObject = [&world](std::int32_t id) {
        return std::find_if(
            world.scenarioObjects().begin(),
            world.scenarioObjects().end(),
            [id](const osf::ScenarioObjectActor& object) {
                return object.id() == id;
            });
    };
    auto first = findObject(1030);
    auto second = findObject(1031);
    if (!check(
            first != world.scenarioObjects().end() &&
                second != world.scenarioObjects().end(),
            "Near Remote Town no longer contains the retail paired "
            "objects.")) {
        return false;
    }
    const std::array<std::int32_t, 3> first_base{{
        first->stateValue(
            osf::ScenarioEntityStateChannel::visible),
        first->stateValue(
            osf::ScenarioEntityStateChannel::pointer),
        first->stateValue(
            osf::ScenarioEntityStateChannel::judgement),
    }};
    const std::array<std::int32_t, 3> second_base{{
        second->stateValue(
            osf::ScenarioEntityStateChannel::visible),
        second->stateValue(
            osf::ScenarioEntityStateChannel::pointer),
        second->stateValue(
            osf::ScenarioEntityStateChannel::judgement),
    }};
    world.update();
    const std::array<std::int32_t, 6> effect_directions{{
        1, 1, 1, 5, 5, 7,
    }};
    const std::array<std::int32_t, 6> effect_bounds{{
        2, 2, 2, 0, 0, 2,
    }};
    if (!check(
            world.combatEffects().size() ==
                effect_directions.size(),
            "Near Remote Town did not create its first retail "
            "placed-effect group.")) {
        return false;
    }
    for (std::size_t index = 0;
         index < effect_directions.size(); ++index) {
        const osf::CombatEffectActor& effect =
            world.combatEffects()[index];
        const osf::ObjectBounds& bounds = effect.judgement();
        if (!check(
                effect.effectNumber() == 20009 &&
                    effect.resourceId() == 11000007 &&
                    effect.direction() == effect_directions[index] &&
                    effect.displayHeight() == 150 &&
                    bounds.left == effect_bounds[index] &&
                    bounds.top == effect_bounds[index] &&
                    bounds.right == effect_bounds[index] &&
                    bounds.bottom == effect_bounds[index],
                "A live opcode-36 effect lost its retail resource, "
                "direction, height, or judgement.")) {
            return false;
        }
    }
    first = findObject(1030);
    second = findObject(1031);
    const std::array<std::int32_t, 3> first_after{{
        first->stateValue(
            osf::ScenarioEntityStateChannel::visible),
        first->stateValue(
            osf::ScenarioEntityStateChannel::pointer),
        first->stateValue(
            osf::ScenarioEntityStateChannel::judgement),
    }};
    const std::array<std::int32_t, 3> second_after{{
        second->stateValue(
            osf::ScenarioEntityStateChannel::visible),
        second->stateValue(
            osf::ScenarioEntityStateChannel::pointer),
        second->stateValue(
            osf::ScenarioEntityStateChannel::judgement),
    }};
    return check(
        first->stateOverrideEnabled() &&
            second->stateOverrideEnabled() &&
            first->visible() && !first->pointerEnabled() &&
            !first->judgementEnabled() &&
            !second->visible() && !second->pointerEnabled() &&
            !second->judgementEnabled() &&
            first_after == first_base &&
            second_after == second_base,
        "Opcode 56 did not swap the paired objects independently of "
        "their script-addressable base state.");
#else
    return true;
#endif
}

bool testScriptedRemoteTownExit() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    osf::WorldScene world;
    std::string error;
    if (!check(
            world.loadInitialScenario(
                data_root, osf::PlayerLoadRequest{}, &error),
            "The authored-exit fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    const auto trigger = std::find_if(
        world.scenarioObjects().begin(),
        world.scenarioObjects().end(),
        [](const osf::ScenarioObjectActor& object) {
            return object.characterNumber() == 10000000;
        });
    if (!check(
            trigger != world.scenarioObjects().end() &&
                !trigger->visible() &&
                !trigger->judgementEnabled(),
            "Remote Town no longer contains its retail invisible "
            "south-gate trigger.")) {
        return false;
    }
    const osf::ObjectBounds& bounds = trigger->judgement();
    const osf::WorldPosition target{
        trigger->position().x +
            (bounds.left + bounds.right) / 2,
        trigger->position().y +
            (bounds.top + bounds.bottom) / 2,
    };
    const osf::ScreenPosition target_screen =
        osf::calculateRealPosition(target);
    world.commandPlayerMovement(
        target_screen.x - world.cameraScreenX(),
        target_screen.y - world.cameraScreenY());
    for (std::int32_t update = 0;
         update < 2000 && world.scenarioId() == 0;
         ++update) {
        world.update();
    }

    const osf::ScenarioEntry* destination_entry =
        world.scenario().findEntry(0);
    if (!check(
            world.scenarioId() == 1 &&
            world.scenario().title() ==
                "Near the Remote Town" &&
            destination_entry &&
            destination_entry->world_x == 90581 &&
            destination_entry->world_y == 5288 &&
            destination_entry->direction == 7 &&
            world.playerWorldX() == destination_entry->world_x &&
            world.playerWorldY() == destination_entry->world_y &&
            world.playerDirection() ==
                destination_entry->direction &&
            world.cameraScreenX() == 12473 &&
            world.cameraScreenY() == 9346 &&
            world.musicTrack() == 1 &&
            world.scenario().mapPath() ==
                "Map\\f00_02.map" &&
            world.ground().width() > 0 &&
            world.objectMap().objects().size() > 0 &&
            world.enemies().size() == 127 &&
            world.enemies().front().aiControl() &&
            world.takeScenarioChanged() &&
            !world.takeScenarioChanged(),
        "Walking through Remote Town's authored trigger did not run "
        "status kind three, opcode 17, and the destination load exactly "
        "once.")) {
        return false;
    }
    const auto return_trigger = std::find_if(
        world.scenarioObjects().begin(),
        world.scenarioObjects().end(),
        [](const osf::ScenarioObjectActor& object) {
            return object.characterNumber() == 10000000;
        });
    if (!check(
            return_trigger != world.scenarioObjects().end(),
            "The first outdoor map no longer contains its authored "
            "Remote Town return trigger.")) {
        return false;
    }
    const osf::ObjectBounds& return_bounds =
        return_trigger->judgement();
    const osf::WorldPosition return_target{
        return_trigger->position().x +
            (return_bounds.left + return_bounds.right) / 2,
        return_trigger->position().y +
            (return_bounds.top + return_bounds.bottom) / 2,
    };
    const osf::ScreenPosition return_screen =
        osf::calculateRealPosition(return_target);
    world.commandPlayerMovement(
        return_screen.x - world.cameraScreenX(),
        return_screen.y - world.cameraScreenY());
    for (std::int32_t update = 0;
         update < 2000 && world.scenarioId() == 1;
         ++update) {
        world.update();
    }

    const osf::ScenarioEntry* town_entry =
        world.scenario().findEntry(0);
    return check(
        world.scenarioId() == 0 &&
            town_entry &&
            town_entry->world_x == 89898 &&
            town_entry->world_y == 2811 &&
            town_entry->direction == 3 &&
            world.playerWorldX() == town_entry->world_x &&
            world.playerWorldY() == town_entry->world_y &&
            world.playerDirection() == town_entry->direction &&
            world.cameraScreenX() == 12743 &&
            world.cameraScreenY() == 9030 &&
            world.musicTrack() == 0 &&
            world.scenarioObjects().size() == 7 &&
            world.enemies().empty() &&
            world.takeScenarioChanged() &&
            !world.takeScenarioChanged(),
        "The outdoor return trigger did not restore Remote Town entry "
        "zero and publish one loading transition.");
#else
    return true;
#endif
}

bool testPlacedScenarioItems() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    const std::filesystem::path scenario_path =
        data_root / "Scenario" / "03130000" /
        "Scenario.Mct";
    if (!std::filesystem::is_regular_file(scenario_path)) {
        return true;
    }

    osf::ScenarioData source;
    std::string error;
    if (!check(
            source.load(scenario_path, &error) &&
                source.items().size() == 4,
            "The authored-item fixture scenario could not be decoded.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::PlayerLoadRequest player;
    player.name = "PlacedItems";
    osf::WorldScene world;
    if (!check(
            world.loadInitialScenario(
                data_root,
                player,
                {3130000, 0, 0},
                &error),
            "The authored-item fixture scenario could not be loaded.")) {
        std::cerr << error << '\n';
        return false;
    }

    if (!check(
            world.groundItems().size() ==
                    source.items().size() &&
                world.takeAudioSamples().empty(),
            "Loading map-authored items emitted audio or lost an "
            "item record.")) {
        return false;
    }
    for (std::size_t index = 0;
         index < source.items().size();
         ++index) {
        const osf::ScenarioItem& expected =
            source.items()[index];
        const osf::GroundItem& item =
            world.groundItems()[index];
        if (!check(
                item.item.category == expected.category &&
                    item.item.definition_id ==
                        expected.definition_id &&
                    item.item.quantity == 1 &&
                    item.position.x == expected.world_x &&
                    item.position.y == expected.world_y &&
                    item.id ==
                        static_cast<std::int32_t>(index) &&
                    item.scenario_character_number ==
                        18000000 + expected.id &&
                    item.resource_id >= 0 &&
                    item.animation_chart >= 0 &&
                    item.height == 0 &&
                    item.vertical_velocity == 0 &&
                    item.bounce_state == 2 &&
                    item.visible() &&
                    item.pointerEnabled() &&
                    item.judgementEnabled() &&
                    item.judgement.left == -20 &&
                    item.judgement.top == -20 &&
                    item.judgement.right == 19 &&
                    item.judgement.bottom == 19,
                "A map-authored item did not preserve its MCT identity, "
                "resolved visual, or settled runtime state.")) {
            return false;
        }
    }

    const osf::GroundItem& first_item =
        world.groundItems().front();
    const osf::ItemWorldResource* static_resource =
        world.itemWorldResource(first_item.resource_id);
    if (!check(
            static_resource &&
                !static_resource->animated() &&
                first_item.animation_chart >= 0 &&
                static_cast<std::size_t>(
                    first_item.animation_chart) <
                    static_resource->patterns()
                        .patterns()
                        .size() &&
                static_cast<std::size_t>(
                    first_item.animation_chart) <
                    static_resource->shadowPatterns()
                        .patterns()
                        .size(),
            "The retail static item-resource layout was not resolved.")) {
        return false;
    }

    NpcRecordingBackend renderer;
    renderer.item_patterns =
        &static_resource->patterns();
    renderer.item_shadows =
        &static_resource->shadowPatterns();
    osf::renderWorldGeometry(renderer, world);
    if (!check(
            renderer.item_calls.size() == 8 &&
                std::all_of(
                    renderer.item_calls.begin(),
                    renderer.item_calls.end(),
                    [&first_item](
                        const NpcPatternCall& call) {
                        return call.pattern ==
                            static_cast<std::size_t>(
                                first_item
                                    .animation_chart);
                    }),
            "Static map-authored item patterns and shadows were not "
            "rendered through their retail chart index.")) {
        return false;
    }

    const osf::ScreenPosition item_anchor =
        osf::calculateRealPosition(first_item.position);
    bool item_hovered = false;
    for (std::int32_t y = -96;
         y <= 64 && !item_hovered;
         ++y) {
        for (std::int32_t x = -96;
             x <= 96;
             ++x) {
            world.updatePointerHover(
                item_anchor.x -
                    world.cameraScreenX() + x,
                item_anchor.y -
                    world.cameraScreenY() + y);
            if (world.hoveredGroundItemId() ==
                first_item.id) {
                item_hovered = true;
                break;
            }
        }
    }
    if (!check(
            item_hovered,
            "A static map-authored item could not be selected through "
            "its visible pattern.")) {
        return false;
    }
    world.clearPointerHover();

    const std::vector<osf::GroundItem> initial_items =
        world.groundItems();
    error.clear();
    if (!check(
            world.transitionScenario(
                {88888888, 0, 0},
                &error) ==
                    osf::ScenarioTravelResult::failed &&
                world.scenarioId() == 3130000 &&
                world.groundItems().size() ==
                    initial_items.size() &&
                !error.empty(),
            "A failed scenario load discarded the live authored-item "
            "owner.")) {
        return false;
    }
    for (std::size_t index = 0;
         index < initial_items.size();
         ++index) {
        if (!check(
                world.groundItems()[index].id ==
                        initial_items[index].id &&
                    world.groundItems()[index]
                            .scenario_character_number ==
                        initial_items[index]
                            .scenario_character_number &&
                    world.groundItems()[index].item.category ==
                        initial_items[index].item.category &&
                    world.groundItems()[index].item.definition_id ==
                        initial_items[index].item.definition_id &&
                    world.groundItems()[index].position.x ==
                        initial_items[index].position.x &&
                    world.groundItems()[index].position.y ==
                        initial_items[index].position.y,
                "A failed scenario load partially replaced an authored "
                "item.")) {
            return false;
        }
    }

    if (!check(
            world.transitionScenario({0, 0, 0}, &error) ==
                    osf::ScenarioTravelResult::loaded &&
                world.scenarioId() == 0 &&
                world.groundItems().empty(),
            "Leaving an authored-item scenario retained its local "
            "ground items.")) {
        std::cerr << error << '\n';
        return false;
    }
    if (!check(
            world.transitionScenario(
                {3130000, 0, 0},
                &error) ==
                    osf::ScenarioTravelResult::loaded &&
                world.groundItems().size() ==
                    source.items().size(),
            "Returning to an authored-item scenario did not rebuild its "
            "local item actors.")) {
        std::cerr << error << '\n';
        return false;
    }
    for (std::size_t index = 0;
         index < world.groundItems().size();
         ++index) {
        if (!check(
                world.groundItems()[index].id ==
                        static_cast<std::int32_t>(index) &&
                    world.groundItems()[index].bounce_state == 2,
                "Reloaded map-authored items did not receive stable "
                "scenario-local selection IDs or settled state.")) {
            return false;
        }
    }
    return true;
#else
    return true;
#endif
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
    const std::array<std::int32_t, 7> object_ids{
        0, 200, 201, 202, 203, 204, 300,
    };
    const std::array<std::int32_t, 7> object_resources{
        -1, 8, 8, -1, 15, 15, 14,
    };
    const std::array<
        std::array<std::int32_t, 3>,
        7>
        object_initial_states{{
            {0, 1, 0},
            {1, 1, 1},
            {1, 0, 0},
            {0, 0, 0},
            {1, 0, 0},
            {1, 0, 0},
            {1, 1, 1},
        }};
    const std::array<
        std::array<std::int32_t, 13>,
        7>
        object_tails{{
            {0, -1, -1, 0, 0, 0, -1,
             0, 1000, 0, 1000, 1000, 1000},
            {1, 0, -1, 0, 0, 0, -1,
             0, 1000, 0, 1000, 1000, 1000},
            {1, 1, -1, 1, 0, 0, -1,
             0, 1000, 0, 1000, 1000, 1000},
            {1, -1, -1, 0, 0, 0, -1,
             0, 1000, 0, 1000, 1000, 1000},
            {0, -1, 0, 0, 1200, 0, -1,
             0, 1000, 0, 1000, 1000, 1000},
            {1, 0, -1, 1, 0, 0, -1,
             16, 0, 0, 1000, 1000, 1000},
            {1, 0, 0, 0, 0, 0, -1,
             0, 1000, 0, 1000, 1000, 1000},
        }};
    bool object_records_match =
        scenario.objects().size() == object_ids.size();
    for (std::size_t index = 0;
         index < scenario.objects().size() &&
         index < object_ids.size();
         ++index) {
        const osf::ScenarioObject& object =
            scenario.objects()[index];
        const std::array<std::int32_t, 13> tail{
            object.visual_mode,
            object.static_pattern,
            object.animation_chart,
            object.draw_status_bit_80 ? 1 : 0,
            object.height,
            object.unknown_tail_5,
            object.unknown_tail_6,
            object.draw_flags,
            object.draw_strength,
            object.unknown_tail_9,
            object.red_draw_strength,
            object.green_draw_strength,
            object.blue_draw_strength,
        };
        object_records_match =
            object_records_match &&
            object.id == object_ids[index] &&
            object.resource_id == object_resources[index] &&
            object.initial_state_values ==
                std::vector<std::int32_t>(
                    object_initial_states[index].begin(),
                    object_initial_states[index].end()) &&
            tail == object_tails[index];
    }
    bool people_tail_matches = scenario.people().size() == 7;
    const std::array<
        std::array<std::int32_t, 3>,
        7>
        people_initial_states{{
            {1, 1, 1},
            {1, 1, 1},
            {1, 1, 1},
            {0, 0, 0},
            {0, 0, 0},
            {0, 0, 0},
            {0, 0, 0},
        }};
    for (std::size_t index = 0;
         index < scenario.people().size();
         ++index) {
        const osf::ScenarioPerson& person =
            scenario.people()[index];
        people_tail_matches =
            people_tail_matches &&
            person.initial_state_values ==
                std::vector<std::int32_t>(
                    people_initial_states[index].begin(),
                    people_initial_states[index].end()) &&
            person.wandering_enabled == (index == 0) &&
            person.scripted_turning_enabled == (index != 1) &&
            person.reserved_behavior_value == -65;
    }
    if (!check(
            scenario.mapPath() == "Map\\f00_01.map" &&
                scenario.title() == "Remote Town" &&
                scenario.musicTrack() == 0 &&
                scenario.objectResourceIds() ==
                    std::vector<std::int32_t>{8, 15, 14} &&
                scenario.peopleResourceIds() ==
                    std::vector<std::int32_t>{
                        13, 8, 9, 1000000, 1000001} &&
                scenario.enemyResourceIds().empty() &&
                object_records_match &&
                scenario.objects()[0].world_x == 90124 &&
                scenario.objects()[0].world_y == 4275 &&
                scenario.objects()[0].judgement_left == -106 &&
                scenario.objects()[0].judgement_bottom == 604 &&
                scenario.objects()[6].name == "  Warehouse  " &&
                scenario.objects()[6].label_height == 80 &&
                scenario.objects()[6].world_x == 92314 &&
                scenario.objects()[6].world_y == 565 &&
                scenario.objects()[6].unknown_common_value == -1 &&
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
                ostare->scripted_turning_enabled &&
                ostare->reserved_behavior_value == -65 &&
                people_tail_matches &&
                scenario.entries().size() == 12 &&
                entry &&
                entry->world_x == 89898 &&
                entry->world_y == 2811 &&
                entry->direction == 3 &&
                scenario.footerValues() ==
                    std::array<std::int32_t, 3>{
                        0, 0, 2000},
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
        world.scenarioId() == 0 &&
            world.scenario().title() == "Remote Town" &&
            world.playerWorldX() == 89898 &&
            world.playerWorldY() == 2811 &&
            world.playerDirection() == 3 &&
            world.playerData().name() == "Mina" &&
            world.playerData().level() == 1 &&
            world.playerData().baseMaximumLife() == 140 &&
            world.playerData().baseMaximumMana() == 160 &&
            world.musicTrack() == 0 &&
            world.vendorInventory(0) &&
            !world.vendorInventory(0)->items().empty() &&
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
            world.npcs()[4].id() == 10001 &&
            world.npcs()[4].name() == "Gravity" &&
            world.npcs()[5].id() == 10002 &&
            world.npcs()[5].name() == "Dune" &&
            world.npcs()[6].id() == 10003 &&
            world.npcs()[6].resourceId() == 1000001 &&
            world.npcs()[6].name() == "Harley",
            "WorldScene did not build the ascending Remote Town PEOPLE "
            "update order.")) {
        return false;
    }
    if (!check(
            world.scenarioObjects().size() == 7 &&
                world.scenarioObjects()[0].id() == 0 &&
                world.scenarioObjects()[0].characterNumber() ==
                    10000000 &&
                !world.scenarioObjects()[0].visible() &&
                world.scenarioObjects()[0].pointerEnabled() &&
                !world.scenarioObjects()[0].judgementEnabled() &&
                world.scenarioObjects()[1].id() == 200 &&
                world.scenarioObjects()[1].resourceId() == 8 &&
                world.scenarioObjects()[1].hasStaticVisual() &&
                world.scenarioObjects()[1].hasStaticShadow() &&
                world.scenarioObjects()[1].drawEnabled() &&
                world.scenarioObjects()[1].judgementEnabled() &&
                world.scenarioObjects()[2].id() == 201 &&
                world.scenarioObjects()[2].visible() &&
                !world.scenarioObjects()[2].pointerEnabled() &&
                !world.scenarioObjects()[2].judgementEnabled() &&
                world.scenarioObjects()[4].id() == 203 &&
                world.scenarioObjects()[4].resourceId() == 15 &&
                world.scenarioObjects()[4].hasAnimatedVisual() &&
                world.scenarioObjects()[4].animationChart() == 0 &&
                world.scenarioObjects()[4].animationFrame() == 0 &&
                world.scenarioObjects()[4].displayHeight() == 240 &&
                world.scenarioObjects()[5].id() == 204 &&
                !world.scenarioObjects()[5].drawEnabled() &&
                world.scenarioObjects()[6].id() == 300 &&
                world.scenarioObjects()[6].resourceId() == 14 &&
                world.scenarioObjects()[6].name() ==
                    "  Warehouse  " &&
                world.scenarioObjects()[6].hasStaticVisual() &&
                world.scenarioObjects()[6].hasStaticShadow() &&
                world.scenarioObjects()[6].pointerEnabled() &&
                world.scenarioObjects()[6].judgementEnabled(),
            "WorldScene did not preserve the retail type-zero object "
            "state, resource mode, or draw controls.")) {
        return false;
    }

    bool initial_medicine_layout = true;
    for (std::int32_t row = 0; row < 4; ++row) {
        const osf::InventoryItem* bag_tablet =
            world.playerInventory().itemAt(0, row);
        const osf::InventoryItem* bag_capsule =
            world.playerInventory().itemAt(1, row);
        const osf::InventoryItem* belt_tablet =
            world.playerBelt().itemAt(row, 0);
        const osf::InventoryItem* belt_capsule =
            world.playerBelt().itemAt(row, 1);
        initial_medicine_layout =
            initial_medicine_layout &&
            bag_tablet &&
            bag_tablet->category == 3 &&
            bag_tablet->definition_id == 0 &&
            bag_capsule &&
            bag_capsule->category == 3 &&
            bag_capsule->definition_id == 10000000 &&
            belt_tablet &&
            belt_tablet->definition_id == 0 &&
            belt_capsule &&
            belt_capsule->definition_id == 10000000;
    }
    const osf::InventoryItem* initial_armor =
        world.playerEquipment().item(
            osf::EquipmentSlot::body);
    if (!check(
            initial_armor &&
                initial_armor->category == 1 &&
                initial_armor->definition_id == 0 &&
                initial_armor->durability == 100 &&
                world.playerPartEnabled(5) &&
                world.playerInventory().items().size() == 8 &&
                world.playerBelt().items().size() == 8 &&
                world.playerMineCount() == 5 &&
                initial_medicine_layout,
            "The retail new-character item loadout differs.")) {
        return false;
    }
    world.playerInventory().clear();
    world.playerEquipment().clear();
    world.playerBelt().clear();
    world.refreshPlayerAppearance();

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
            renderer.calls.size() == 4 &&
                renderer.calls[0].shadow &&
                renderer.calls[0].pattern == 280 &&
                renderer.calls[0].draw.x == 747 &&
                renderer.calls[0].draw.y == 269 &&
                renderer.calls[0].draw.opacity == 500 &&
                !renderer.calls[1].shadow &&
                renderer.calls[1].pattern == 1744 &&
                renderer.calls[1].draw.red_strength == 1000 &&
                !renderer.calls[2].shadow &&
                renderer.calls[2].pattern == 1784 &&
                !renderer.calls[3].shadow &&
                renderer.calls[3].pattern == 280,
            "Ostare's idle frame, part mask, shadow, or placement differs.")) {
        return false;
    }
    NpcRecordingBackend warehouse_renderer;
    warehouse_renderer.patterns =
        &world.scenarioObjects()[6].staticPatterns();
    warehouse_renderer.shadows =
        &world.scenarioObjects()[6].staticShadows();
    osf::renderWorld(warehouse_renderer, world, 500);
    if (!check(
            warehouse_renderer.calls.size() == 2 &&
                warehouse_renderer.calls[0].shadow &&
                warehouse_renderer.calls[0].pattern == 0 &&
                warehouse_renderer.calls[0].draw.opacity == 500 &&
                !warehouse_renderer.calls[1].shadow &&
                warehouse_renderer.calls[1].pattern == 0 &&
                warehouse_renderer.calls[1].draw.opacity == 1000,
            "The Warehouse type-zero object did not submit its retail "
            "shadow and static pattern passes.")) {
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
                renderer.calls.size() == 4 &&
                renderer.calls[1].draw.red_strength == 1300 &&
                renderer.calls[1].draw.green_strength == 1300 &&
                renderer.calls[1].draw.blue_strength == 1300 &&
                renderer.calls[3].draw.red_strength == 1300 &&
                renderer.calls[3].draw.green_strength == 1300 &&
                renderer.calls[3].draw.blue_strength == 1300 &&
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
                world.npcs()[0].direction() ==
                    osf::retailDirectionForVector(
                        world.playerWorldX() -
                            world.npcs()[0].position().x,
                        world.playerWorldY() -
                            world.npcs()[0].position().y) &&
                world.conversationText().rfind(
                    "Thank you for coming. I am Ostare", 0) == 0,
            "Ostare's click did not enter and face the player through "
            "its retail status-zero script.")) {
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
    world.takeAudioSamples();
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
                world.groundItems()[0].item.category == 0 &&
                world.groundItems()[0].item.definition_id == 0 &&
                world.groundItems()[0].item.quantity == 1 &&
                world.groundItems()[0].position.x ==
                    interaction_position.x + 200 &&
                world.groundItems()[0].position.y ==
                    interaction_position.y &&
                world.groundItems()[0].resource_id == 0 &&
                world.groundItems()[0].animation_chart == 0 &&
                world.groundItems()[0].red_strength == 1000 &&
                world.groundItems()[0].green_strength == 1000 &&
                world.groundItems()[0].blue_strength == 1000 &&
                world.groundItems()[1].item.category == 1 &&
                world.groundItems()[1].item.definition_id == 1000000 &&
                world.groundItems()[1].position.x ==
                    interaction_position.x &&
                world.groundItems()[1].position.y ==
                    interaction_position.y + 200 &&
                world.groundItems()[1].resource_id == 0 &&
                world.groundItems()[1].animation_chart == 5 &&
                world.groundItems()[1].red_strength == 900 &&
                world.groundItems()[1].green_strength == 800 &&
                world.groundItems()[1].blue_strength == 500 &&
                world.groundItems()[2].item.category == 0 &&
                world.groundItems()[2].item.definition_id == 100 &&
                world.groundItems()[2].position.x ==
                    interaction_position.x + 200 &&
                world.groundItems()[2].position.y ==
                    interaction_position.y - 200 &&
                world.groundItems()[2].resource_id == 0 &&
                world.groundItems()[2].animation_chart == 36 &&
                world.groundItems()[2].red_strength == 1000 &&
                world.groundItems()[2].green_strength == 1000 &&
                world.groundItems()[2].blue_strength == 1000 &&
                world.groundItems()[3].item.category == 4 &&
                world.groundItems()[3].item.definition_id == 0 &&
                world.groundItems()[3].item.quantity == 200 &&
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
    for (std::int32_t update = 0; update < 19; ++update) {
        world.update();
    }
    const std::vector<std::int32_t> drop_samples =
        world.takeAudioSamples();
    if (!check(
            std::count(
                drop_samples.begin(),
                drop_samples.end(),
                15) == 3 &&
                std::count(
                    drop_samples.begin(),
                    drop_samples.end(),
                    85) == 1,
            "Ostare's scripted drops did not emit their retail landing sounds.")) {
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
                renderer.item_calls.size() == 12 &&
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
                renderer.item_calls[4].pattern == 36 &&
                renderer.item_calls[4].draw.palette == 73 &&
                renderer.item_calls[4].draw.x ==
                    renderer.item_calls[0].draw.x &&
                renderer.item_calls[4].draw.y ==
                    renderer.item_calls[0].draw.y &&
                renderer.item_calls[5].pattern == 113 &&
                renderer.item_calls[5].draw.palette == 72 &&
                renderer.item_calls[5].draw.x ==
                    renderer.item_calls[0].draw.x &&
                renderer.item_calls[5].draw.y ==
                    renderer.item_calls[0].draw.y &&
                renderer.item_calls[6].pattern == 0 &&
                renderer.item_calls[6].draw.palette == 1 &&
                renderer.item_calls[6].draw.x ==
                    renderer.item_calls[1].draw.x &&
                renderer.item_calls[6].draw.y ==
                    renderer.item_calls[1].draw.y &&
                renderer.item_calls[7].pattern == 77 &&
                renderer.item_calls[7].draw.palette == 0 &&
                renderer.item_calls[7].draw.x ==
                    renderer.item_calls[1].draw.x &&
                renderer.item_calls[7].draw.y ==
                    renderer.item_calls[1].draw.y &&
                renderer.item_calls[8].pattern == 5 &&
                renderer.item_calls[8].draw.palette == 11 &&
                renderer.item_calls[8].draw.x ==
                    renderer.item_calls[2].draw.x &&
                renderer.item_calls[8].draw.y ==
                    renderer.item_calls[2].draw.y &&
                renderer.item_calls[9].pattern == 82 &&
                renderer.item_calls[9].draw.palette == 10 &&
                renderer.item_calls[9].draw.red_strength == 900 &&
                renderer.item_calls[9].draw.green_strength == 800 &&
                renderer.item_calls[9].draw.blue_strength == 500 &&
                renderer.item_calls[9].draw.x ==
                    renderer.item_calls[2].draw.x &&
                renderer.item_calls[9].draw.y ==
                    renderer.item_calls[2].draw.y &&
                renderer.item_calls[10].pattern == 30 &&
                renderer.item_calls[10].draw.palette == 61 &&
                renderer.item_calls[10].draw.x ==
                    renderer.item_calls[3].draw.x &&
                renderer.item_calls[10].draw.y ==
                    renderer.item_calls[3].draw.y &&
                renderer.item_calls[11].pattern == 107 &&
                renderer.item_calls[11].draw.palette == 60 &&
                renderer.item_calls[11].draw.x ==
                    renderer.item_calls[3].draw.x &&
                renderer.item_calls[11].draw.y ==
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
            findGroundItemRangeOnlyPoint(
                world,
                short_sword_id,
                short_sword_pointer),
            "The Short Sword was not selectable through the "
            "configured click-range square.")) {
        return false;
    }
    osf::WorldPointerConfiguration pointer_configuration =
        world.pointerConfiguration();
    pointer_configuration.range_enabled = false;
    world.configurePointer(pointer_configuration);
    world.updatePointerHover(
        short_sword_pointer.x, short_sword_pointer.y);
    if (!check(
            world.hoveredGroundItemId() != short_sword_id,
            "Disabling the click range did not restore exact-tip "
            "ground-item picking.")) {
        return false;
    }
    pointer_configuration.range_enabled = true;
    world.configurePointer(pointer_configuration);
    world.updatePointerHover(
        short_sword_pointer.x, short_sword_pointer.y);
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
    const osf::ItemDefinition* pickup_blocker =
        world.itemDatabase().find(3, 0);
    if (!check(
            pickup_blocker &&
                world.playerInventory().add(
                    *pickup_blocker,
                    osf::PlayerInventory::grid_width *
                        osf::PlayerInventory::grid_height),
            "The full-backpack pickup fixture could not be prepared.")) {
        return false;
    }
    const bool short_sword_click =
        world.commandWorldInteraction(
            short_sword_pointer.x,
            short_sword_pointer.y);
    for (std::int32_t update = 0;
         update < 2000 &&
         world.groundItems().front().bounce_state == 2;
         ++update) {
        world.update();
    }
    if (!check(
            short_sword_click &&
                world.groundItems().size() == 4 &&
                world.groundItems().front().id ==
                    short_sword_id &&
                world.groundItems().front().bounce_state == 0 &&
                world.groundItems().front().height == 0 &&
                world.groundItems().front().vertical_velocity == 1600 &&
                world.playerInventory().items().size() ==
                    static_cast<std::size_t>(
                        osf::PlayerInventory::grid_width *
                        osf::PlayerInventory::grid_height),
            "A full backpack did not leave the clicked item on the "
            "ground and restart its retail drop animation.")) {
        return false;
    }
    world.playerInventory().clear();
    bool heard_rejected_pickup_landing = false;
    for (std::int32_t update = 0;
         update < 40 &&
         world.groundItems().front().bounce_state != 2;
         ++update) {
        world.update();
        const std::vector<std::int32_t> samples =
            world.takeAudioSamples();
        heard_rejected_pickup_landing =
            heard_rejected_pickup_landing ||
            std::find(
                samples.begin(), samples.end(), 15) !=
                samples.end();
    }
    if (!check(
            world.groundItems().front().bounce_state == 2 &&
                heard_rejected_pickup_landing,
            "The rejected pickup did not complete its bounce and "
            "play the Short Sword landing sound.")) {
        return false;
    }
    if (!check(
            findGroundItemRangeOnlyPoint(
                world,
                short_sword_id,
                short_sword_pointer) &&
                world.commandWorldInteraction(
                    short_sword_pointer.x,
                    short_sword_pointer.y),
            "The settled rejected pickup could not be selected again.")) {
        return false;
    }
    for (std::int32_t update = 0;
         update < 2000 &&
         world.groundItems().size() == 4;
         ++update) {
        world.update();
    }
    if (!check(
            world.groundItems().size() == 3 &&
                world.playerInventory().items().size() == 1 &&
                world.playerInventory().items()[0].category == 0 &&
                world.playerInventory()
                        .items()[0]
                        .definition_id == 0 &&
                world.playerInventory().items()[0].quantity == 1 &&
                world.playerInventory().items()[0].width ==
                    world.itemDatabase()
                        .find(0, 0)
                        ->inventory_width &&
                world.playerInventory().items()[0].height ==
                    world.itemDatabase()
                        .find(0, 0)
                        ->inventory_height &&
                world.playerInventory().items()[0].durability ==
                    world.itemDatabase()
                        .find(0, 0)
                        ->maximum_durability,
            "The retail approach-and-pickup path did not transfer "
            "the Short Sword into player inventory.")) {
        return false;
    }
    const std::int32_t drop_origin_x =
        world.playerWorldX();
    const std::int32_t drop_origin_y =
        world.playerWorldY();
    const std::optional<osf::InventoryItem> dropped_sword =
        world.playerInventory().take(0);
    if (!check(
            dropped_sword &&
                world.dropInventoryItem(
                    *dropped_sword, 0, 240) &&
                world.playerInventory().items().empty() &&
                world.groundItems().size() == 4 &&
                world.groundItems().back().item.category == 0 &&
                world.groundItems().back().item.definition_id == 0 &&
                world.groundItems().back().resource_id == 0 &&
                (std::abs(
                     world.groundItems().back().position.x -
                     drop_origin_x) == 200 ||
                 std::abs(
                     world.groundItems().back().position.y -
                     drop_origin_y) == 200),
            "Dropping an owned item did not return it through the "
            "retail ground-item path.")) {
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
                world.conversationMessageId() == 1000019 &&
                malse.direction() == 1,
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
    const std::vector<std::int32_t> quest_audio =
        world.takeAudioSamples();
    if (!check(
            world.conversationActive() &&
                world.conversationActorId() == 2 &&
                world.conversationMessageId() == 1000041 &&
                world.quests().state(0) == 1 &&
                world.quests().lastCue() ==
                    osf::QuestCue::updated &&
                world.quests().notice().quest_id == 0 &&
                world.quests().notice().counter == 600 &&
                std::find(
                    quest_audio.begin(),
                    quest_audio.end(),
                    65) != quest_audio.end(),
            "Syria's callback did not apply its retail quest update, "
            "notice, and sample 65 cue.")) {
        return false;
    }
    world.advanceConversation();
    if (!check(
            !world.conversationActive() &&
                world.conversationActorId() == -1,
            "Syria's opening conversation did not release world control.")) {
        return false;
    }
    world.takeAudioSamples();
    osf::ScreenPosition syria_repeat_pointer;
    const bool syria_repeat_click =
        findNpcPointerPoint(
            world, 2, syria_repeat_pointer) &&
        world.commandWorldInteraction(
            syria_repeat_pointer.x,
            syria_repeat_pointer.y);
    updateUntilConversation(world);
    const std::vector<std::int32_t> syria_repeat_audio =
        world.takeAudioSamples();
    if (!check(
            syria_repeat_click &&
                world.conversationActive() &&
                world.conversationActorId() == 2 &&
                world.conversationMessageId() == 1000038 &&
                world.quests().state(0) == 1 &&
                std::find(
                    syria_repeat_audio.begin(),
                    syria_repeat_audio.end(),
                    65) == syria_repeat_audio.end(),
            "Syria's repeat interaction restarted quest zero instead "
            "of entering her normal blessing dialogue.")) {
        std::cerr
            << "repeat-click=" << syria_repeat_click
            << " active=" << world.conversationActive()
            << " actor=" << world.conversationActorId()
            << " message=" << world.conversationMessageId()
            << " quest=" << world.quests().state(0)
            << " notice=" << world.quests().notice().counter
            << " life=" << world.playerCurrentLife()
            << '/' << world.playerRuntimeProfile().maximum_life
            << " mana=" << world.playerCurrentMana()
            << '/' << world.playerRuntimeProfile().maximum_mana
            << '\n';
        return false;
    }
    world.advanceConversation();
    if (!check(
            !world.conversationActive(),
            "Syria's repeat blessing did not return world control.")) {
        return false;
    }

    const std::int32_t player_maximum_life =
        world.playerRuntimeProfile().maximum_life;
    const std::int32_t player_maximum_mana =
        world.playerRuntimeProfile().maximum_mana;
    osf::PlayerData& wounded_player =
        const_cast<osf::PlayerData&>(world.playerData());
    wounded_player.setCurrentLife(1, player_maximum_life);
    wounded_player.setCurrentMana(1, player_maximum_mana);
    if (!check(
            world.hasCompanion(),
            "The Syria recovery fixture has no owned companion.")) {
        return false;
    }
    osf::CompanionActor& wounded_companion =
        const_cast<osf::CompanionActor&>(world.companion());
    osf::CompanionDamageReceiverState companion_state =
        wounded_companion.damageReceiverState();
    companion_state.current_life = 1;
    wounded_companion.applyDamageReceiverState(companion_state);
    world.takeAudioSamples();

    osf::ScreenPosition syria_blessing_pointer;
    const bool syria_blessing_click =
        findNpcPointerPoint(
            world, 2, syria_blessing_pointer) &&
        world.commandWorldInteraction(
            syria_blessing_pointer.x,
            syria_blessing_pointer.y);
    updateUntilConversation(world);
    if (!check(
            syria_blessing_click &&
                world.conversationActive() &&
                world.conversationMessageId() == 1000037,
            "A wounded player did not reach Syria's retail recovery "
            "message.")) {
        return false;
    }
    world.advanceConversation();
    const std::vector<std::int32_t> blessing_audio =
        world.takeAudioSamples();
    if (!check(
            !world.conversationActive() &&
                world.playerCurrentLife() == player_maximum_life &&
                world.playerCurrentMana() == player_maximum_mana &&
                world.companion().currentLife() ==
                    world.companion().maximumLife() &&
                world.npcs()[2].animationChart() == 3 &&
                world.npcs()[2].animationFrame() == 0 &&
                !blessing_audio.empty(),
            "Syria's retail callback did not restore the living party, "
            "start PEOPLE chart three, and play its authored sample.")) {
        return false;
    }

    const osf::NpcActor& blessing_syria = world.npcs()[2];
    const osf::gapi::CafDirection& blessing_direction =
        blessing_syria.animation().charts()[3].directions[
            static_cast<std::size_t>(blessing_syria.direction())];
    for (std::int32_t update = 0;
         update < blessing_direction.frame_count;
         ++update) {
        world.update();
    }
    if (!check(
            blessing_syria.animationChart() == 3 &&
                blessing_syria.animationFrame() ==
                    blessing_direction.frame_count - 1,
            "Syria's one-shot blessing did not retain its retail final "
            "CAF frame.")) {
        return false;
    }
    world.update();
    if (!check(
            blessing_syria.animationChart() == 0 &&
                blessing_syria.animationFrame() == 0,
            "Syria did not return to PEOPLE idle after her one-shot "
            "blessing.")) {
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
    for (const osf::ScenarioObjectActor& object :
         companion_world.scenarioObjects()) {
        if (!object.judgementEnabled()) {
            continue;
        }
        town_actor_blockers.push_back({
            object.movementBlockerId(),
            object.position(),
            object.judgement(),
        });
    }
    for (const osf::NpcActor& npc : companion_world.npcs()) {
        if (!npc.judgementEnabled()) {
            continue;
        }
        town_actor_blockers.push_back({
            npc.movementBlockerId(),
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

    const std::int32_t object_animation_frame =
        companion_world.scenarioObjects()[4].animationFrame();
    companion_world.update();
    const osf::NpcActor& kerberos =
        companion_world.npcs()[3];
    const osf::NpcActor& gravity =
        companion_world.npcs()[4];
    if (!check(
            !kerberos.visible() &&
                !kerberos.pointerEnabled() &&
                !kerberos.judgementEnabled() &&
                gravity.visible() &&
                gravity.pointerEnabled() &&
                gravity.judgementEnabled() &&
                companion_world.npcs()[5].visible() &&
                companion_world.npcs()[5].pointerEnabled() &&
                companion_world.npcs()[5].judgementEnabled() &&
                companion_world.npcs()[6].visible() &&
                companion_world.npcs()[6].pointerEnabled() &&
                companion_world.npcs()[6].judgementEnabled() &&
                object_animation_frame == 0 &&
                companion_world.scenarioObjects()[4]
                        .animationFrame() == 1,
            "The periodic companion scripts did not hide the player's "
            "own dog, activate the other town companions, or advance "
            "type-zero actors in retail order.")) {
        return false;
    }
    osf::ScreenPosition kerberos_pointer;
    if (!check(
            !findNpcPointerPoint(
                companion_world, kerberos.id(), kerberos_pointer),
            "The player's own companion remained selectable in town.")) {
        return false;
    }
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
    constexpr osf::WorldPosition gravity_approach[] = {
        {93000, 1500},
        {93000, 0},
        {94000, -1500},
        {93200, -3200},
        {89900, -3200},
    };
    for (const osf::WorldPosition waypoint : gravity_approach) {
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
    osf::ScreenPosition gravity_pointer;
    if (!check(
            findNpcPointerPoint(
                companion_world, gravity.id(), gravity_pointer),
            "Gravity has no opaque retail pointer cell.")) {
        return false;
    }
    const bool gravity_click =
        companion_world.commandWorldInteraction(
            gravity_pointer.x,
            gravity_pointer.y);
    const bool gravity_started_or_approached =
        companion_world.interactionPending() ||
        companion_world.conversationActive();
    if (!check(
            gravity_click &&
                gravity_started_or_approached &&
                updateUntilConversation(companion_world, 5000) &&
                companion_world.conversationActorId() == 10001 &&
                companion_world.conversationRequiresSelection() &&
                companion_world.conversationInitialSelection() == 3,
            "The retail movement controller did not approach Gravity "
            "and open his choice message.")) {
        std::cerr
            << "Player: "
            << companion_world.playerWorldX() << ", "
            << companion_world.playerWorldY()
            << "; Gravity: "
            << gravity.position().x << ", "
            << gravity.position().y << '\n';
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
            "Gravity's rendered choice message is missing a range.")) {
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
    companion_world.chooseConversationOption(0);
    if (!check(
            companion_world.conversationActive() &&
                !companion_world.conversationRequiresSelection() &&
                companion_world.conversationActorId() == 10001 &&
                companion_world.conversationText().find(
                    "Gravity\n\nLevel") == 0 &&
                companion_world.conversationText().find(
                    "Attribute") != std::string::npos &&
                companion_world.conversationText().find(
                    "Attack Speed") != std::string::npos &&
                companion_world.conversationText().find(
                    "Experience") != std::string::npos,
            "Gravity's Check Status choice did not open its retail "
            "table-backed speech bubble.")) {
        return false;
    }
    NpcRecordingBackend status_renderer;
    status_renderer.speech = &companion_world.speechPatterns();
    osf::renderWorld(
        status_renderer,
        companion_world,
        500,
        &font);
    if (!check(
            std::any_of(
                status_renderer.text_calls.begin(),
                status_renderer.text_calls.end(),
                [&companion_world](const TextCall& call) {
                    return call.text ==
                        companion_world.conversationText();
                }),
            "Gravity's companion status text was not rendered in the "
            "actor speech bubble.")) {
        return false;
    }
    companion_world.advanceConversation();
    if (!check(
            !companion_world.conversationActive() &&
                companion_world.conversationActorId() == -1,
            "Closing Gravity's status did not release the actor through "
            "the authored status-one branch.")) {
        return false;
    }
    if (!check(
            companion_world.commandWorldInteraction(
                gravity_pointer.x,
                gravity_pointer.y) &&
                updateUntilConversation(companion_world, 5000) &&
                companion_world.conversationRequiresSelection(),
            "Gravity's companion menu could not be reopened after Check "
            "Status.")) {
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
            "Gravity's rendered QUIT choice was not clickable.")) {
        return false;
    }

    if (!check(
            companion_world.commandWorldInteraction(
                gravity_pointer.x,
                gravity_pointer.y) &&
                updateUntilConversation(companion_world, 5000) &&
                companion_world.conversationRequiresSelection(),
            "Gravity's companion menu could not be reopened for a live "
            "swap.")) {
        return false;
    }
    companion_world.chooseConversationOption(2);
    if (!check(
            !companion_world.conversationActive() &&
                companion_world.playerData().companionType() == 1 &&
                companion_world.playerData().companionLevel() == 1 &&
                companion_world.hasCompanion() &&
                companion_world.companion().profile().type == 1 &&
                companion_world.companion().profile().name == "Gravity" &&
                companion_world.companion().currentLife() ==
                    companion_world.companion().maximumLife() &&
                companion_world.companion().position().x ==
                    companion_world.playerWorldX() &&
                companion_world.companion().position().y ==
                    companion_world.playerWorldY(),
            "Opcode 45 did not replace the owned companion at the player "
            "with full life and its own saved profile.")) {
        return false;
    }
    companion_world.update();
    if (!check(
            companion_world.npcs()[3].visible() &&
                companion_world.npcs()[3].pointerEnabled() &&
                companion_world.npcs()[3].judgementEnabled() &&
                !companion_world.npcs()[4].visible() &&
                !companion_world.npcs()[4].pointerEnabled() &&
                !companion_world.npcs()[4].judgementEnabled() &&
                companion_world.npcs()[5].visible() &&
                companion_world.npcs()[6].visible(),
            "Remote Town's periodic scripts did not exchange the old and "
            "new town companion actors after a swap.")) {
        return false;
    }
    const std::filesystem::path companion_save_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_companion_swap_test";
    const std::filesystem::path companion_save_path =
        companion_save_root / "Save" / "0000.Ssv";
    std::error_code companion_cleanup_error;
    std::filesystem::remove_all(
        companion_save_root, companion_cleanup_error);
    if (!check(
            osf::writeRetailSave(
                companion_save_path,
                companion_world.playerData(),
                companion_world.itemDatabase(),
                companion_world.playerInventory(),
                companion_world.playerEquipment(),
                companion_world.playerBelt(),
                companion_world.playerSpecialItems(),
                companion_world.retailSaveProgress(),
                companion_world.playerMagic(),
                companion_world.playerMineCount(),
                companion_world.retailSaveWorldState(),
                companion_world.playerGiantWarehouse(),
                companion_world.playerAutomaticItems(),
                0x34,
                &error),
            "The swapped companion state could not be written to a retail "
            "save.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerLoadRequest companion_save_request;
    companion_save_request.source =
        osf::PlayerDataSource::retail_save;
    companion_save_request.save_path = companion_save_path;
    osf::WorldScene restored_companion_world;
    const bool restored_companion =
        restored_companion_world.loadInitialScenario(
            data_root, companion_save_request, &error);
    std::filesystem::remove_all(
        companion_save_root, companion_cleanup_error);
    if (!check(
            restored_companion &&
                restored_companion_world.playerData().companionType() == 1 &&
                restored_companion_world.hasCompanion() &&
                restored_companion_world.companion().profile().type == 1 &&
                restored_companion_world.companion().profile().name ==
                    "Gravity" &&
                restored_companion_world.companion().currentLife() ==
                    restored_companion_world.companion().maximumLife(),
            "The selected companion or its live full-life actor did not "
            "survive save and load.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene harley_world;
    if (!check(
            harley_world.loadInitialScenario(
                data_root, osf::PlayerLoadRequest{}, &error),
            "Remote Town could not be reloaded for Harley's dialogue.")) {
        return false;
    }
    harley_world.update();
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

    osf::ScreenPosition transport_pointer;
    if (!check(
            world.transports().destinations().size() == 51 &&
                world.transports().enabled(0) &&
                world.activateTransportDestination(0) ==
                    osf::ScenarioTravelResult::relocated &&
                world.playerWorldX() == 94685 &&
                world.playerWorldY() == -2756 &&
                world.playerDirection() == 7 &&
                findScenarioObjectPointerPoint(
                    world, 200, transport_pointer) &&
                world.hoveredScenarioObjectId() == 200,
            "Remote Town's type-zero transport object was not selectable "
            "at its retail entry point.")) {
        return false;
    }
    if (!check(
            world.commandWorldInteraction(
                transport_pointer.x,
                transport_pointer.y),
            "The transport object's pointer click was not accepted.")) {
        return false;
    }
    osf::GameplayServiceRequest transport_request =
        world.takeGameplayServiceRequest();
    for (std::int32_t update = 0;
         update < 2000 &&
         transport_request.kind ==
             osf::GameplayServiceKind::none;
         ++update) {
        world.update();
        transport_request =
            world.takeGameplayServiceRequest();
    }
    if (!check(
            transport_request.kind ==
                    osf::GameplayServiceKind::transport &&
                transport_request.argument == 0 &&
                !world.interactionPending(),
            "The transport object did not route through status zero and "
            "opcode 37.")) {
        return false;
    }

    osf::WorldScene wander_world;
    if (!check(
            wander_world.loadInitialScenario(
                data_root, osf::PlayerLoadRequest{}, &error),
            "Remote Town could not be reloaded for the wander check.")) {
        return false;
    }
    const osf::ScreenPosition ostare_screen =
        osf::calculateRealPosition(
            wander_world.npcs()[0].position());
    const std::int32_t ostare_view_x =
        ostare_screen.x - wander_world.cameraScreenX();
    const std::int32_t ostare_view_y =
        ostare_screen.y - wander_world.cameraScreenY();
    if (!check(
            ostare_view_x < 0 ||
                ostare_view_x >= 640 ||
                ostare_view_y < 0 ||
                ostare_view_y >= 480,
            "The offscreen PEOPLE update fixture unexpectedly placed "
            "Ostare inside the camera.")) {
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
        "Offscreen Ostare did not begin the retail bounded wander "
        "action.");
#else
    return true;
#endif
}

}  // namespace

int main() {
    return testNpcScriptAction() &&
                   testScenarioEffectCommand() &&
                   testScenarioPlacedEffectCommand() &&
                   testGroundItemCreation() &&
                   testConversationChoiceMarkup() &&
                   testPlayerLevelUpNoticeLayout() &&
                   testEnemyNameplatePresentation() &&
                   testFixture() &&
                   testMalformedData() &&
                   testRetailScenarioCatalog() &&
                   testGeneralScenarioStart() &&
                   testLiveScenarioTransition() &&
                   testRetailScenarioEntryInitialization() &&
                   testRetailScenarioObjectOverride() &&
                   testScriptedRemoteTownExit() &&
                   testPlacedScenarioItems() &&
                   testWorldItemSaveRoundTrip() &&
                   testPersistentConversationAndMovementState() &&
                   testLegacyDeadSaveRecovery() &&
                   testRetailRemoteTown()
               ? 0
               : 1;
}
