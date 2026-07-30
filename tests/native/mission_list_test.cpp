#include "core/game_config.hpp"
#include "gapi/gapi.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "render/gameplay_mission_list_renderer.hpp"
#include "render/gameplay_transport_renderer.hpp"
#include "states/gameplay_mission_list.hpp"
#include "states/gameplay_options_menu.hpp"
#include "states/gameplay_transport.hpp"
#include "world/mission_catalog.hpp"
#include "world/quest_state.hpp"
#include "world/transport_catalog.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct PatternCall {
    std::size_t index = 0;
    osf::gapi::PatternDraw draw;
};

struct TextCall {
    std::string text;
    osf::gapi::TextDraw draw;
};

class RecordingBackend final : public osf::gapi::Backend {
public:
    void beginFrame(osf::gapi::Color) override {}

    bool drawPattern(
        const osf::gapi::NjpImage&,
        std::size_t pattern_index,
        const osf::gapi::PatternDraw& draw) override {
        patterns.push_back({pattern_index, draw});
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
        texts.push_back({std::string(text), draw});
        return true;
    }

    bool drawRectangle(
        const osf::gapi::RectangleDraw&) override {
        return true;
    }

    void endFrame() override {}

    std::vector<PatternCall> patterns;
    std::vector<TextCall> texts;
};

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool loadCatalog(osf::MissionCatalog& catalog) {
    osf::TableDatabase tables;
    std::string error;
    const std::string path =
        std::string(OPENSHADOWFLARE_SOURCE_DIR) +
        "/tmp/ShadowFlare/System/Game/Parameter/Table.Tbd";
    return check(
        tables.load(path, &error) &&
            catalog.load(tables, &error),
        error.empty()
            ? "The retail mission catalog could not be loaded."
            : error.c_str());
}

bool testMissionCatalog() {
    osf::MissionCatalog catalog;
    if (!loadCatalog(catalog)) {
        return false;
    }
    const osf::MissionDefinition* first = catalog.find(0);
    const osf::MissionDefinition* last = catalog.find(47);
    if (!check(
            catalog.missions().size() == 48 &&
                first &&
                first->title == "Defeat the Red Goblin." &&
                first->description.size() == 5 &&
                first->description[0] ==
                    "The Red Goblin living in the northeast of the" &&
                last &&
                last->title == "Go to the Forest of Madness.",
            "Mission titles or description tables differ from retail.")) {
        return false;
    }
    catalog.clear();
    return check(
        catalog.missions().empty() && !catalog.find(0),
        "Clearing the mission catalog retained table-owned data.");
}

bool testMissionListState() {
    osf::QuestState quests;
    quests.applyScriptUpdate(0, 1);
    quests.applyScriptUpdate(24, 1);
    const auto mission_visible =
        [&quests](std::int32_t mission_id) {
            return quests.state(mission_id) != 0;
        };

    osf::GameplayMissionList list;
    list.open();
    if (!check(
            list.active() &&
                list.page() == 0 &&
                list.selectedMission() == -1,
            "Opening the Mission List did not reset its detail view.")) {
        return false;
    }

    osf::GameplayMissionListResult result =
        list.update(
            {false, false, true, 60, 50},
            mission_visible);
    if (!check(
            !result.play_move_sound &&
                list.selectedMission() == 0,
            "An active mission row could not open its details.")) {
        return false;
    }
    list.update(
        {false, false, true, 320, 200},
        mission_visible);
    if (!check(
            list.active() && list.selectedMission() == -1,
            "A detail click did not return to the Mission List.")) {
        return false;
    }

    result = list.update(
        {false, false, true, 350, 20},
        mission_visible);
    if (!check(
            result.play_move_sound && list.page() == 1,
            "The second retail mission page could not be selected.")) {
        return false;
    }
    list.update(
        {false, false, true, 60, 50},
        mission_visible);
    if (!check(
            list.selectedMission() == 24,
            "The second page did not map its first row to mission 24.")) {
        return false;
    }
    list.update(
        {false, true, false, 0, 0},
        mission_visible);
    if (!check(
            !list.active(),
            "Escape did not close the Mission List.")) {
        return false;
    }

    list.update(
        {true, false, false, 0, 0},
        mission_visible);
    list.update(
        {false, false, true, 20, 450},
        mission_visible);
    if (!check(
            list.active(),
            "A lower-HUD click incorrectly dismissed the Mission List.")) {
        return false;
    }
    list.update(
        {false, false, true, 20, 200},
        mission_visible);
    return check(
        !list.active(),
        "A click outside the retail list controls was not dismissed.");
}

bool testMissionListRendering() {
    osf::MissionCatalog catalog;
    if (!loadCatalog(catalog)) {
        return false;
    }
    osf::QuestState quests;
    quests.applyScriptUpdate(0, 1);
    quests.applyScriptUpdate(12, 1);
    quests.applyScriptUpdate(12, 2);

    osf::GameplayMissionList list;
    list.open();
    const auto mission_visible =
        [&quests](std::int32_t mission_id) {
            return quests.state(mission_id) != 0;
        };
    osf::gapi::NjpImage status;
    osf::gapi::NjpImage font;
    RecordingBackend backend;
    osf::renderGameplayMissionList(
        backend,
        status,
        font,
        list,
        catalog,
        quests);

    const auto active_title = std::find_if(
        backend.texts.begin(),
        backend.texts.end(),
        [](const TextCall& call) {
            return call.text == "Defeat the Red Goblin." &&
                   call.draw.x == 79 &&
                   call.draw.y == 54 &&
                   call.draw.color.red == 224;
        });
    const auto completed_title = std::find_if(
        backend.texts.begin(),
        backend.texts.end(),
        [](const TextCall& call) {
            return call.text ==
                       "Head for the Mining Tunnel of Yugunos." &&
                   call.draw.x == 373 &&
                   call.draw.y == 54 &&
                   call.draw.color.red == 128;
        });
    if (!check(
            backend.patterns.size() == 5 &&
                backend.patterns[0].index == 10 &&
                backend.patterns[1].index == 112 &&
                backend.patterns[2].index == 111 &&
                backend.patterns[3].index == 25 &&
                backend.patterns[3].draw.x == 52 &&
                backend.patterns[3].draw.y == 48 &&
                backend.patterns[4].index == 26 &&
                backend.patterns[4].draw.x == 346 &&
                backend.patterns[4].draw.y == 48 &&
                active_title != backend.texts.end() &&
                completed_title != backend.texts.end(),
            "The mission frame, page tabs, locks, or titles differ "
            "from retail.")) {
        return false;
    }

    list.update(
        {false, false, true, 60, 50},
        mission_visible);
    backend = {};
    osf::renderGameplayMissionList(
        backend,
        status,
        font,
        list,
        catalog,
        quests);
    const auto detail_title = std::find_if(
        backend.texts.begin(),
        backend.texts.end(),
        [](const TextCall& call) {
            return call.text == "[Defeat the Red Goblin.]" &&
                   call.draw.x == 190 &&
                   call.draw.y == 96 &&
                   call.draw.color.red == 224 &&
                   call.draw.color.green == 192 &&
                   call.draw.color.blue == 0;
        });
    const auto detail_line = std::find_if(
        backend.texts.begin(),
        backend.texts.end(),
        [](const TextCall& call) {
            return call.text ==
                       "The Red Goblin living in the northeast of the" &&
                   call.draw.x == 190 &&
                   call.draw.y == 120;
        });
    return check(
        backend.patterns.size() == 7 &&
            backend.patterns[5].index == 59 &&
            backend.patterns[5].draw.red_strength == 800 &&
            backend.patterns[5].draw.blue_strength == 1200 &&
            backend.patterns[6].index == 58 &&
            detail_title != backend.texts.end() &&
            detail_line != backend.texts.end(),
        "The selected mission detail panel differs from retail.");
}

bool testOptionsEntry() {
    osf::GameplayOptionsMenu menu;
    osf::GameConfig config;
    menu.update({true, false, false, 0, 0}, config);
    const osf::GameplayOptionsResult result =
        menu.update({false, true, true, 300, 254}, config);
    return check(
        result.action ==
                osf::GameplayOptionsAction::open_mission_list &&
            result.play_confirm_sound,
        "The Settings menu Mission List row is not wired.");
}

bool testTransportPanel() {
    osf::TableDatabase tables;
    std::string error;
    if (!check(
            tables.load(
                std::string(OPENSHADOWFLARE_SOURCE_DIR) +
                    "/tmp/ShadowFlare/System/Game/Parameter/Table.Tbd",
                &error),
            "The retail tables for the transport panel could not be loaded.")) {
        return false;
    }
    osf::TransportCatalog catalog;
    if (!check(
            catalog.load(tables, &error) &&
                catalog.destinations().size() == 51 &&
                catalog.enabledRows() ==
                    std::vector<std::int32_t>{0} &&
                catalog.destinations()[0].name == "Remote Town" &&
                catalog.destinations()[0].scenario == 0 &&
                catalog.destinations()[0].entry == 50,
            "The transport owner did not preserve retail Table 40.")) {
        return false;
    }

    osf::GameplayTransport panel;
    panel.open();
    const std::vector<std::int32_t> enabled =
        catalog.enabledRows();
    const osf::GameplayTransportResult hovered =
        panel.update({false, false, 40, 64}, enabled);
    if (!check(
            !hovered.pointer_consumed &&
                panel.active() &&
                panel.page() == 0 &&
                panel.pageCount(enabled.size()) == 1 &&
                panel.hoveredDestination() == 0 &&
                panel.visibleDestinations(enabled) == enabled,
            "The transport panel did not map its first retail hit row.")) {
        return false;
    }

    osf::gapi::NjpImage status;
    osf::gapi::NjpImage font;
    RecordingBackend backend;
    osf::renderGameplayTransport(
        backend, status, font, panel, catalog);
    const auto title = std::find_if(
        backend.texts.begin(),
        backend.texts.end(),
        [](const TextCall& call) {
            return call.text == "Remote Town" &&
                   call.draw.x == 79 &&
                   call.draw.y == 65 &&
                   call.draw.color.red == 224;
        });
    if (!check(
            backend.patterns.size() == 2 &&
                backend.patterns[0].index == 13 &&
                backend.patterns[1].index == 23 &&
                backend.patterns[1].draw.x == 52 &&
                backend.patterns[1].draw.y == 59 &&
                title != backend.texts.end(),
            "The transport frame, hovered row, or destination title "
            "differs from retail.")) {
        return false;
    }

    const osf::GameplayTransportResult selected =
        panel.update({false, true, 40, 64}, enabled);
    if (!check(
            selected.selected_destination == 0 &&
                selected.play_move_sound &&
                selected.pointer_consumed &&
                !panel.active(),
            "Selecting a transport destination did not close and consume "
            "the retail panel click.")) {
        return false;
    }

    std::vector<std::int32_t> two_pages;
    for (std::int32_t row = 0; row < 11; ++row) {
        two_pages.push_back(row);
    }
    panel.open();
    const osf::GameplayTransportResult next =
        panel.update({false, true, 230, 370}, two_pages);
    return check(
        next.play_move_sound &&
            next.selected_destination == -1 &&
            panel.page() == 1 &&
            panel.visibleDestinations(two_pages) ==
                std::vector<std::int32_t>{10},
        "The retail ten-destination transport paging differs.");
}

}  // namespace

int main() {
    return testMissionCatalog() &&
                   testMissionListState() &&
                   testMissionListRendering() &&
                   testOptionsEntry() &&
                   testTransportPanel()
        ? 0
        : 1;
}
