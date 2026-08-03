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

osf::RetailSaveProgress dragonProgress(
    const osf::WorldScene& seed) {
    osf::RetailSaveProgress progress = seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    for (const std::int32_t completed :
         {0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 13, 14, 15, 16}) {
        progress.quest_flags[
            static_cast<std::size_t>(completed)] = 2;
    }
    progress.quest_flags[12] = 1;
    progress.quest_flags[17] = 1;
    if (progress.script_state_flags.size() < 72) {
        progress.script_state_flags.resize(72, 0);
    }
    progress.script_state_flags[11] = 2;
    progress.script_state_flags[15] = 1;
    progress.script_state_flags[23] = 1;
    progress.script_state_flags[24] = 1;
    progress.script_state_flags[38] = 1;
    progress.script_state_flags[39] = 2;
    progress.script_state_flags[40] = 1;
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
        0x57,
        &error);
}

bool waitForMissionCompletion(
    osf::WorldScene& world,
    std::vector<std::int32_t>& audio,
    std::int32_t maximum_updates = 1000) {
    for (std::int32_t update = 0;
         update < maximum_updates && world.quests().state(17) != 2;
         ++update) {
        world.update();
        const std::vector<std::int32_t> samples =
            world.takeAudioSamples();
        audio.insert(audio.end(), samples.begin(), samples.end());
    }
    return world.quests().state(17) == 2;
}

bool testDragons(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "Dragons";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The dragon fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerData level_sixty = seed.playerData();
    if (!check(
            raiseToLevel(level_sixty, 60, seed.parameterTables()),
            "The dragon fixture could not reach its area level.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_two_dragons_test";
    const std::filesystem::path active_save =
        fixture_root / "active" / "Save" / "0000.Ssv";
    const std::filesystem::path completed_save =
        fixture_root / "completed" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);

    if (!check(
            writeFixture(
                active_save,
                seed,
                level_sixty,
                dragonProgress(seed),
                {true, 2200000, 0},
                error),
            "The active dragon mission fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene world;
    if (!check(
            loadSavedFixture(data_root, active_save, world, error) &&
                world.quests().state(16) == 2 &&
                world.quests().state(17) == 1 &&
                osf::test::openNpcConversation(world, 4) &&
                world.conversationMessageId() == 1000070,
            "Kirarru did not begin the authored seal preparation.")) {
        std::cerr << error << " message="
                  << world.conversationMessageId() << '\n';
        return false;
    }
    for (const std::int32_t message : {1000071, 1000072}) {
        world.advanceConversation();
        if (!check(
                world.conversationMessageId() == message,
                "Kirarru's seal preparation skipped a message.")) {
            std::cerr << "message=" << world.conversationMessageId()
                      << " expected=" << message << '\n';
            return false;
        }
    }
    world.advanceConversation();
    world.advanceConversation();
    if (!check(
            !world.conversationActive() &&
                world.quests().state(17) == 1,
            "Kirarru's preparation changed the active mission.")) {
        return false;
    }

    if (!check(
            world.transitionScenario({2210003, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::loaded &&
                world.scenario().title() ==
                    "Mining Tunnel of Yugnos, B5F",
            "The active dragon mission could not reach B5F.")) {
        std::cerr << error << " scenario=" << world.scenarioId()
                  << " title=" << world.scenario().title() << '\n';
        return false;
    }
    if (!check(
            osf::test::markScenarioEnemiesDefeated(
                world, 0, 89),
            "The B5F route fixture could not isolate the dragon exit.")) {
        std::cerr << "enemies=" << world.enemies().size() << '\n';
        return false;
    }
    if (!check(
            osf::test::walkThroughScenarioTrigger(
                world, 1, 2210004) &&
                world.scenario().title() ==
                    "Mining Tunnel of Yugunos, B5F",
            "The active dragon mission did not open the B5F exit.")) {
        std::cerr << "scenario=" << world.scenarioId()
                  << " title=" << world.scenario().title() << '\n';
        return false;
    }

    const auto ancient_dragon = std::find_if(
        world.enemies().begin(),
        world.enemies().end(),
        [](const osf::EnemyActor& enemy) {
            return enemy.id() == 10000 &&
                   enemy.name() == "Ancient Dragon";
        });
    if (!check(
            ancient_dragon != world.enemies().end() &&
                osf::test::markScenarioEnemiesDefeated(
                    world, 10000, 10000),
            "The authored Ancient Dragon objective could not be defeated.")) {
        return false;
    }
    std::vector<std::int32_t> completion_audio;
    if (!check(
            waitForMissionCompletion(world, completion_audio) &&
                world.quests().lastCue() ==
                    osf::QuestCue::completed &&
                containsSample(completion_audio, 66),
            "The Ancient Dragon's completed death did not finish mission seventeen.")) {
        return false;
    }

    if (!check(
            osf::test::walkThroughScenarioTrigger(
                world, 0, 2210003) &&
                world.retailSaveWorldState().entry_value == 1,
            "The dragon chamber did not return to the authored B5F entry.")) {
        return false;
    }
    if (!check(
            writeFixture(
                completed_save,
                world,
                world.playerData(),
                world.retailSaveProgress(),
                {true, 2200000, 0},
                error),
            "The completed dragon mission could not return to Fanann.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene lytle_report;
    if (!check(
            loadSavedFixture(
                data_root, completed_save, lytle_report, error) &&
                lytle_report.quests().state(17) == 2 &&
                osf::test::openNpcConversation(lytle_report, 0) &&
                lytle_report.conversationMessageId() == 1000018 &&
                lytle_report.retailSaveProgress()
                        .script_state_flags[41] == 2,
            "Lytle did not accept the completed dragon report.")) {
        std::cerr << error << " message="
                  << lytle_report.conversationMessageId() << '\n';
        return false;
    }
    lytle_report.advanceConversation();
    lytle_report.advanceConversation();

    osf::WorldScene kirarru_report;
    if (!check(
            loadSavedFixture(
                data_root, completed_save, kirarru_report, error) &&
                osf::test::openNpcConversation(kirarru_report, 4) &&
                kirarru_report.conversationMessageId() == 1000073 &&
                kirarru_report.quests().state(17) == 2,
            "Kirarru did not acknowledge the defeated dragons.")) {
        std::cerr << error << " message="
                  << kirarru_report.conversationMessageId() << '\n';
        return false;
    }
    kirarru_report.advanceConversation();
    kirarru_report.advanceConversation();

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
            data_root / "Scenario" / "02210004")) {
        return 0;
    }
    return testDragons(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
