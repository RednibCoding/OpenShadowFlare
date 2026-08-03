#include "episode_one_test_support.hpp"
#include "world/retail_save_file.hpp"
#include "world/retail_save_progress.hpp"
#include "world/retail_save_world_state.hpp"
#include "world/world_scene.hpp"

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

osf::RetailSaveProgress yugunosProgress(const osf::WorldScene& seed) {
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
    if (progress.script_state_flags.size() < 72) {
        progress.script_state_flags.resize(72, 0);
    }
    progress.script_state_flags[11] = 2;
    progress.script_state_flags[15] = 1;
    progress.script_state_flags[23] = 1;
    progress.script_state_flags[24] = 1;
    progress.script_state_flags[41] = 1;
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
        0x53,
        &error);
}

bool walkUntilScriptFlag(
    osf::WorldScene& world,
    std::int32_t object_id,
    std::size_t flag_index,
    std::int32_t expected_value,
    std::int32_t maximum_updates = 5000) {
    const osf::ScenarioObjectActor* trigger =
        osf::test::findScenarioTrigger(world, object_id);
    if (!trigger) {
        return false;
    }
    const osf::WorldPosition target =
        osf::test::scenarioTriggerCenter(*trigger);
    for (std::int32_t update = 0;
         update < maximum_updates;
         ++update) {
        const osf::RetailSaveProgress& progress =
            world.retailSaveProgress();
        if (flag_index < progress.script_state_flags.size() &&
            progress.script_state_flags[flag_index] == expected_value) {
            return true;
        }
        if (update % 30 == 0) {
            const osf::ScreenPosition screen =
                osf::calculateRealPosition(target);
            world.commandPlayerMovement(
                screen.x - world.cameraScreenX(),
                screen.y - world.cameraScreenY());
        }
        world.update();
        world.takeAudioSamples();
    }
    return false;
}

bool testYugunosBlockade(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "YugunosBlockade";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The Yugunos fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerData level_sixty = seed.playerData();
    if (!check(
            raiseToLevel(level_sixty, 60, seed.parameterTables()),
            "The Yugunos fixture could not reach its area level.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_two_yugunos_blockade_test";
    const std::filesystem::path route_save =
        fixture_root / "route" / "Save" / "0000.Ssv";
    const std::filesystem::path investigated_save =
        fixture_root / "investigated" / "Save" / "0000.Ssv";
    const std::filesystem::path report_save =
        fixture_root / "report" / "Save" / "0000.Ssv";
    const std::filesystem::path briefed_save =
        fixture_root / "briefed" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);

    if (!check(
            writeFixture(
                route_save,
                seed,
                level_sixty,
                yugunosProgress(seed),
                {true, 2200003, 0},
                error),
            "The Dragon Road fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene route;
    if (!check(
            loadSavedFixture(data_root, route_save, route, error) &&
                route.scenario().title() == "Dragon Road" &&
                route.transitionScenario(
                    {2200003, 2, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 2, 2210000) &&
                route.scenario().title() ==
                    "Mining Tunnel of Yugunos, B1F" &&
                route.transitionScenario(
                    {2210000, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 1, 2210001) &&
                route.scenario().title() ==
                    "Mining Tunnel of Yugunos, B2F",
            "Dragon Road did not reach the Yugunos B2F investigation.")) {
        std::cerr << error << '\n';
        return false;
    }

    if (!check(
            route.transitionScenario(
                {2210001, 2, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                walkUntilScriptFlag(route, 2, 38, 1) &&
                route.scenarioId() == 2210001 &&
                route.retailSaveWorldState().entry_value == 2 &&
                route.quests().state(12) == 1 &&
                route.quests().state(15) == 0,
            "The B2F protection did not record and reject the investigation.")) {
        return false;
    }

    if (!check(
            route.transitionScenario(
                {2210001, 0, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 0, 2210000) &&
                route.retailSaveWorldState().entry_value == 1 &&
                route.transitionScenario(
                    {2210000, 0, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 0, 2200003) &&
                route.retailSaveWorldState().entry_value == 2 &&
                route.scenario().title() == "Dragon Road" &&
                route.retailSaveProgress().script_state_flags[38] == 1,
            "The rejected Yugunos route did not return to Dragon Road.")) {
        std::cerr << error << '\n';
        return false;
    }

    if (!check(
            writeFixture(
                investigated_save,
                route,
                route.playerData(),
                route.retailSaveProgress(),
                route.retailSaveWorldState(),
                error),
            "The Yugunos investigation could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene persisted;
    if (!check(
        loadSavedFixture(data_root, investigated_save, persisted, error) &&
            persisted.scenarioId() == 2200003 &&
            persisted.retailSaveWorldState().entry_value == 2 &&
            persisted.retailSaveProgress().script_state_flags[38] == 1 &&
            persisted.retailSaveProgress().script_state_flags[39] == 0 &&
            persisted.retailSaveProgress().script_state_flags[40] == 0 &&
            persisted.quests().state(12) == 1 &&
            persisted.quests().state(15) == 0,
        "Saving the first Yugunos investigation lost its blockade state.")) {
        return false;
    }

    if (!check(
            writeFixture(
                report_save,
                persisted,
                persisted.playerData(),
                persisted.retailSaveProgress(),
                {true, 2200000, 0},
                error),
            "The Yugunos investigation could not return to Kirarru.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene fanann;
    if (!check(
            loadSavedFixture(data_root, report_save, fanann, error) &&
                osf::test::openNpcConversation(fanann, 4) &&
                fanann.conversationMessageId() == 1000048,
            "Kirarru did not begin her authored first meeting.")) {
        std::cerr << error << " message="
                  << fanann.conversationMessageId() << '\n';
        return false;
    }
    for (const std::int32_t message : {1000049, 1000050}) {
        fanann.advanceConversation();
        if (!check(
                fanann.conversationMessageId() == message,
                "Kirarru's first meeting skipped a message.")) {
            return false;
        }
    }
    fanann.advanceConversation();
    if (!check(
            !fanann.conversationActive() &&
                fanann.retailSaveProgress().script_state_flags[45] == 1,
            "Kirarru did not save her first-meeting branch.")) {
        return false;
    }

    std::vector<std::int32_t> briefing_audio;
    if (!check(
            osf::test::openNpcConversation(
                fanann, 4, &briefing_audio) &&
                fanann.conversationMessageId() == 1000052,
            "Kirarru did not accept the B2F blockade report.")) {
        std::cerr << "message=" << fanann.conversationMessageId() << '\n';
        return false;
    }
    for (const std::int32_t message :
         {1000053, 1000054, 1000055}) {
        fanann.advanceConversation();
        if (!check(
                fanann.conversationMessageId() == message,
                "Kirarru's blockade report skipped a message.")) {
            return false;
        }
    }
    fanann.advanceConversation();
    fanann.advanceConversation();
    const std::vector<std::int32_t> update_audio =
        fanann.takeAudioSamples();
    briefing_audio.insert(
        briefing_audio.end(), update_audio.begin(), update_audio.end());
    if (!check(
            !fanann.conversationActive() &&
                fanann.quests().state(12) == 1 &&
                fanann.quests().state(15) == 1 &&
                fanann.retailSaveProgress().script_state_flags[39] == 0 &&
                fanann.retailSaveProgress().script_state_flags[40] == 0 &&
                containsSample(briefing_audio, 65),
            "Kirarru did not start the control-room mission once.")) {
        return false;
    }

    if (!check(
            writeFixture(
                briefed_save,
                fanann,
                fanann.playerData(),
                fanann.retailSaveProgress(),
                fanann.retailSaveWorldState(),
                error),
            "Kirarru's control-room briefing could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene briefed;
    std::vector<std::int32_t> repeat_audio;
    if (!check(
            loadSavedFixture(data_root, briefed_save, briefed, error) &&
                briefed.quests().state(15) == 1 &&
                briefed.retailSaveProgress().script_state_flags[39] == 0 &&
                briefed.retailSaveProgress().script_state_flags[40] == 0 &&
                osf::test::openNpcConversation(
                    briefed, 4, &repeat_audio) &&
                briefed.conversationMessageId() == 1000051 &&
                !containsSample(repeat_audio, 65),
            "Saving Kirarru's briefing replayed the mission update.")) {
        std::cerr << error << " message="
                  << briefed.conversationMessageId() << '\n';
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
            data_root / "Scenario" / "02210000")) {
        return 0;
    }
    return testYugunosBlockade(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
