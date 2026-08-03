#include "episode_one_test_support.hpp"
#include "world/retail_save_file.hpp"
#include "world/retail_save_progress.hpp"
#include "world/retail_save_world_state.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

using osf::test::check;
using osf::test::containsSample;
using osf::test::loadSavedFixture;
using osf::test::raiseToLevel;

osf::RetailSaveProgress controlRoomProgress(
    const osf::WorldScene& seed) {
    osf::RetailSaveProgress progress = seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    for (const std::int32_t completed :
         {0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 13, 14}) {
        progress.quest_flags[
            static_cast<std::size_t>(completed)] = 2;
    }
    progress.quest_flags[12] = 1;
    progress.quest_flags[15] = 1;
    if (progress.script_state_flags.size() < 72) {
        progress.script_state_flags.resize(72, 0);
    }
    progress.script_state_flags[11] = 2;
    progress.script_state_flags[15] = 1;
    progress.script_state_flags[23] = 1;
    progress.script_state_flags[24] = 1;
    progress.script_state_flags[38] = 1;
    progress.script_state_flags[39] = 0;
    progress.script_state_flags[41] = 1;
    progress.script_state_flags[45] = 1;
    progress.script_state_flags[71] = 1;
    return progress;
}

bool writeFixture(
    const std::filesystem::path& save_path,
    const osf::WorldScene& world,
    const osf::PlayerData& player,
    const osf::RetailSaveProgress& progress,
    const osf::RetailSaveWorldState& world_state,
    std::string& error) {
    return osf::writeRetailSave(
        save_path,
        player,
        world.itemDatabase(),
        world.playerInventory(),
        world.playerEquipment(),
        world.playerBelt(),
        world.playerSpecialItems(),
        progress,
        world.playerMagic(),
        world.playerMineCount(),
        world_state,
        world.playerGiantWarehouse(),
        world.playerAutomaticItems(),
        0x55,
        &error);
}

bool waitForMissionCompletion(
    osf::WorldScene& world,
    std::vector<std::int32_t>& audio,
    std::int32_t maximum_updates = 800) {
    for (std::int32_t update = 0;
         update < maximum_updates && world.quests().state(15) != 2;
         ++update) {
        world.update();
        const std::vector<std::int32_t> samples =
            world.takeAudioSamples();
        audio.insert(audio.end(), samples.begin(), samples.end());
    }
    return world.quests().state(15) == 2;
}

bool testControlRoomRoute(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "ControlRoom";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The control-room fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerData level_sixty = seed.playerData();
    if (!check(
            raiseToLevel(level_sixty, 60, seed.parameterTables()),
            "The control-room fixture could not reach its area level.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_two_control_room_test";
    const std::filesystem::path route_save =
        fixture_root / "route" / "Save" / "0000.Ssv";
    const std::filesystem::path return_save =
        fixture_root / "return" / "Save" / "0000.Ssv";
    const std::filesystem::path opened_save =
        fixture_root / "opened" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);

    if (!check(
            writeFixture(
                route_save,
                seed,
                level_sixty,
                controlRoomProgress(seed),
                {true, 2200003, 0},
                error),
            "The Dragon Road mission fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene route;
    if (!check(
            loadSavedFixture(data_root, route_save, route, error) &&
                route.scenario().title() == "Dragon Road" &&
                route.transitionScenario(
                    {2200003, 3, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 3, 2220000) &&
                route.scenario().title() ==
                    "Underground Passage, B1F" &&
                route.retailSaveWorldState().entry_value == 0,
            "Dragon Road did not enter the southern underground passage.")) {
        std::cerr << error << '\n';
        return false;
    }

    if (!check(
            route.transitionScenario(
                {2220000, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 1, 2220001) &&
                route.scenario().title() ==
                    "Underground Passage, B2F" &&
                route.retailSaveWorldState().entry_value == 0,
            "The underground passage stair did not reach B2F.")) {
        std::cerr << error << '\n';
        return false;
    }

    const auto black_wing = std::find_if(
        route.enemies().begin(),
        route.enemies().end(),
        [](const osf::EnemyActor& enemy) {
            return enemy.id() == 10000 &&
                   enemy.name() == "Black Wing";
        });
    if (!check(
            black_wing != route.enemies().end() &&
                route.quests().state(15) == 1 &&
                osf::test::markScenarioEnemiesDefeated(
                    route, 10000, 10000),
            "The authored Black Wing objective could not be defeated.")) {
        return false;
    }

    std::vector<std::int32_t> completion_audio;
    if (!check(
            waitForMissionCompletion(route, completion_audio) &&
                route.quests().lastCue() ==
                    osf::QuestCue::completed &&
                containsSample(completion_audio, 66) &&
                route.quests().state(12) == 1,
            "Black Wing's completed death did not finish mission fifteen.")) {
        std::cerr << "quest15=" << route.quests().state(15)
                  << " sample66="
                  << containsSample(completion_audio, 66) << '\n';
        return false;
    }

    if (!check(
            route.transitionScenario(
                {2220001, 0, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 0, 2220000) &&
                route.retailSaveWorldState().entry_value == 1 &&
                osf::test::walkThroughScenarioTrigger(
                    route, 0, 2200003) &&
                route.scenario().title() == "Dragon Road" &&
                route.retailSaveWorldState().entry_value == 3,
            "The completed control-room route did not return to Dragon Road.")) {
        std::cerr << error << '\n';
        return false;
    }

    if (!check(
            writeFixture(
                return_save,
                route,
                route.playerData(),
                route.retailSaveProgress(),
                {true, 2200000, 0},
                error),
            "The completed control-room mission could not return to Fanann.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene fanann;
    if (!check(
            loadSavedFixture(data_root, return_save, fanann, error) &&
                fanann.quests().state(15) == 2 &&
                fanann.retailSaveProgress().script_state_flags[40] == 0 &&
                osf::test::openNpcConversation(fanann, 4) &&
                fanann.conversationMessageId() == 1000056,
            "Kirarru did not recognize the recaptured control room.")) {
        std::cerr << error << " message="
                  << fanann.conversationMessageId() << '\n';
        return false;
    }
    fanann.advanceConversation();
    if (!check(
            fanann.conversationMessageId() == 1000057,
            "Kirarru's emergency-device response skipped its second line.")) {
        std::cerr << "message=" << fanann.conversationMessageId() << '\n';
        return false;
    }
    fanann.advanceConversation();
    if (!check(
            !fanann.conversationActive() &&
                fanann.retailSaveProgress().script_state_flags[40] == 1 &&
                fanann.quests().state(15) == 2,
            "Kirarru did not save the emergency-device cancellation.")) {
        return false;
    }

    if (!check(
            writeFixture(
                opened_save,
                fanann,
                fanann.playerData(),
                fanann.retailSaveProgress(),
                {true, 2210001, 2},
                error),
            "The opened Yugunos blockade could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene opened;
    if (!check(
            loadSavedFixture(data_root, opened_save, opened, error) &&
                opened.scenarioId() == 2210001 &&
                opened.retailSaveProgress().script_state_flags[38] == 1 &&
                opened.retailSaveProgress().script_state_flags[40] == 1 &&
                opened.quests().state(15) == 2 &&
                osf::test::walkThroughScenarioTrigger(
                    opened, 1, 2210002) &&
                opened.scenario().title() ==
                    "Mining Tunnel of Yugnos, B3F",
            "Saved flag forty did not clear the Yugunos B2F blockade.")) {
        std::cerr << error << " scenario=" << opened.scenarioId()
                  << " entry="
                  << opened.retailSaveWorldState().entry_value << '\n';
        return false;
    }

    std::filesystem::remove_all(fixture_root, cleanup_error);
    return true;
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "02220001")) {
        return 0;
    }
    return testControlRoomRoute(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
