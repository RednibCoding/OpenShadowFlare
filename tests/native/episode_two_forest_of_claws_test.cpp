#include "episode_one_test_support.hpp"
#include "world/retail_save_file.hpp"
#include "world/retail_save_progress.hpp"
#include "world/retail_save_world_state.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace {

using osf::test::check;
using osf::test::containsSample;
using osf::test::loadSavedFixture;
using osf::test::raiseToLevel;

osf::RetailSaveProgress episodeTwoProgress(
    const osf::WorldScene& seed) {
    osf::RetailSaveProgress progress = seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    for (const std::int32_t completed :
         {0, 1, 2, 3, 4, 6, 7, 8, 9, 10}) {
        progress.quest_flags[
            static_cast<std::size_t>(completed)] = 2;
    }
    if (progress.script_state_flags.size() < 72) {
        progress.script_state_flags.resize(72, 0);
    }
    progress.script_state_flags[11] = 2;
    progress.script_state_flags[15] = 1;
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
        0x4d,
        &error);
}

bool authoredOakKnightGroup(const osf::WorldScene& world) {
    std::int32_t first_group = 0;
    std::int32_t second_group = 0;
    for (const osf::EnemyActor& enemy : world.enemies()) {
        if (enemy.name() != "Oak Knight" ||
            enemy.lootTableRow() != 85 ||
            enemy.goldDropChance() != 10) {
            continue;
        }
        if (enemy.id() >= 10000 && enemy.id() <= 10002) {
            ++first_group;
        }
        if (enemy.id() >= 20000 && enemy.id() <= 20002) {
            ++second_group;
        }
    }
    return first_group == 3 && second_group == 3;
}

std::int32_t groundGold(const osf::WorldScene& world) {
    return std::accumulate(
        world.groundItems().begin(),
        world.groundItems().end(),
        0,
        [](std::int32_t total, const osf::GroundItem& item) {
            return item.item.category == 4 &&
                           item.item.definition_id == 0
                       ? total + item.item.quantity
                       : total;
        });
}

bool testForestOfClaws(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "ForestClaws";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The Forest of Claws fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerData level_sixty = seed.playerData();
    if (!check(
            raiseToLevel(level_sixty, 60, seed.parameterTables()),
            "The Forest of Claws fixture could not reach its area level.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_two_forest_of_claws_test";
    const std::filesystem::path offer_save =
        fixture_root / "offer" / "Save" / "0000.Ssv";
    const std::filesystem::path completed_save =
        fixture_root / "completed" / "Save" / "0000.Ssv";
    const std::filesystem::path persisted_save =
        fixture_root / "persisted" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);

    if (!check(
            writeFixture(
                offer_save,
                seed,
                level_sixty,
                episodeTwoProgress(seed),
                {true, 2100000, 0},
                error),
            "The Mining Town mission fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene offered;
    if (!check(
            loadSavedFixture(data_root, offer_save, offered, error) &&
                osf::test::openNpcConversation(offered, 0) &&
                offered.conversationMessageId() == 1000002,
            "Kyle did not begin his authored introduction.")) {
        std::cerr << error << '\n';
        return false;
    }
    for (const std::int32_t message :
         {1000003, 1000004, 1000005, 1000006, 1000007, 1000008}) {
        offered.advanceConversation();
        if (!check(
                offered.conversationMessageId() == message,
                "Kyle's Forest of Claws briefing skipped a message.")) {
            std::cerr << "message="
                      << offered.conversationMessageId()
                      << " expected=" << message << '\n';
            return false;
        }
    }
    const std::vector<std::int32_t> offer_audio =
        offered.takeAudioSamples();
    const osf::MissionDefinition* first_mission =
        offered.missions().find(11);
    if (!check(
            first_mission &&
                first_mission->title ==
                    "Destroy thieves staying SE of Kanfore." &&
                offered.quests().state(11) == 1 &&
                offered.quests().lastCue() ==
                    osf::QuestCue::updated &&
                offered.quests().notice().quest_id == 11 &&
                offered.quests().notice().counter == 600 &&
                containsSample(offer_audio, 65),
            "Kyle did not start mission eleven with its notice and cue.")) {
        std::cerr << "quest=" << offered.quests().state(11)
                  << " notice="
                  << offered.quests().notice().quest_id << '/'
                  << offered.quests().notice().counter << '\n';
        return false;
    }
    offered.advanceConversation();
    if (!check(
            !offered.conversationActive(),
            "Kyle did not release the mission briefing.")) {
        return false;
    }

    if (!check(
            offered.transitionScenario(
                {2100000, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    offered, 1, 2100001) &&
                offered.scenario().title() ==
                    "Forest of Four Leaves" &&
                offered.retailSaveWorldState().entry_value == 0 &&
                offered.transitionScenario(
                    {2100001, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    offered, 1, 2100002) &&
                offered.scenario().title() == "Forest of Claws" &&
                offered.retailSaveWorldState().entry_value == 0,
            "Mining Town's authored southeast route did not reach Forest of Claws.")) {
        std::cerr << "scenario=" << offered.scenarioId()
                  << " title=" << offered.scenario().title()
                  << " entry="
                  << offered.retailSaveWorldState().entry_value
                  << '\n';
        return false;
    }

    if (!check(
            offered.quests().state(11) == 1 &&
                authoredOakKnightGroup(offered) &&
                osf::test::scriptedObjectVisible(
                    offered, 10000700, true) &&
                osf::test::scriptedObjectVisible(
                    offered, 10000701, true) &&
                osf::test::markScenarioEnemiesDefeated(
                    offered, 10000, 10002),
            "The first Forest of Claws guard group differs from retail.")) {
        return false;
    }

    for (std::int32_t update = 0;
         update < 400 &&
         osf::test::scriptedObjectVisible(
             offered, 10000700, true);
         ++update) {
        offered.update();
        offered.takeAudioSamples();
    }
    if (!check(
            offered.quests().state(11) == 1 &&
                osf::test::scriptedObjectVisible(
                    offered, 10000700, false) &&
                osf::test::scriptedObjectVisible(
                    offered, 10000701, false) &&
                osf::test::markScenarioEnemiesDefeated(
                    offered, 20000, 20002),
            "The first Oak Knight clear did not open the inner gate.")) {
        return false;
    }

    std::vector<std::int32_t> completion_audio;
    for (std::int32_t update = 0;
         update < 400 && offered.quests().state(11) != 2;
         ++update) {
        offered.update();
        const std::vector<std::int32_t> samples =
            offered.takeAudioSamples();
        completion_audio.insert(
            completion_audio.end(), samples.begin(), samples.end());
    }
    if (!check(
            offered.quests().state(11) == 2 &&
                offered.quests().lastCue() ==
                    osf::QuestCue::completed &&
                containsSample(completion_audio, 66),
            "The second Oak Knight clear did not complete mission eleven.")) {
        return false;
    }

    if (!check(
            writeFixture(
                completed_save,
                offered,
                offered.playerData(),
                offered.retailSaveProgress(),
                {true, 2100000, 0},
                error),
            "The completed Forest of Claws mission could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene rewarded;
    if (!check(
            loadSavedFixture(
                data_root, completed_save, rewarded, error) &&
                osf::test::openNpcConversation(rewarded, 0) &&
                rewarded.conversationMessageId() == 1000010 &&
                rewarded.groundItems().size() == 2 &&
                groundGold(rewarded) == 20000,
            "Kyle did not drop the authored 20,000-Gold reward.")) {
        std::cerr << "message=" << rewarded.conversationMessageId()
                  << " items=" << rewarded.groundItems().size()
                  << " gold=" << groundGold(rewarded) << '\n';
        return false;
    }
    rewarded.takeAudioSamples();
    for (std::int32_t update = 0; update < 19; ++update) {
        rewarded.update();
    }
    const std::vector<std::int32_t> reward_audio =
        rewarded.takeAudioSamples();
    if (!check(
            std::count(
                reward_audio.begin(), reward_audio.end(), 85) == 2,
            "Kyle's two Gold stacks did not play their landing sounds.")) {
        return false;
    }

    for (const std::int32_t message :
         {1000011, 1000012}) {
        rewarded.advanceConversation();
        if (!check(
                rewarded.conversationMessageId() == message,
                "Kyle's mining-tunnel briefing skipped a message.")) {
            std::cerr << "message="
                      << rewarded.conversationMessageId()
                      << " expected=" << message << '\n';
            return false;
        }
    }
    rewarded.advanceConversation();
    const std::vector<std::int32_t> next_audio =
        rewarded.takeAudioSamples();
    const osf::MissionDefinition* next_mission =
        rewarded.missions().find(12);
    if (!check(
            next_mission &&
                next_mission->title ==
                    "Head for the Mining Tunnel of Yugunos." &&
                !rewarded.conversationActive() &&
                rewarded.quests().state(12) == 1 &&
                rewarded.quests().notice().quest_id == 12 &&
                rewarded.quests().notice().counter == 600 &&
                containsSample(next_audio, 65),
            "Kyle did not hand off the authored mining-tunnel mission.")) {
        return false;
    }

    if (!check(
            writeFixture(
                persisted_save,
                rewarded,
                rewarded.playerData(),
                rewarded.retailSaveProgress(),
                {true, 2100000, 0},
                error),
            "The Episode 2 mission handoff could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene persisted;
    const bool no_repeat =
        loadSavedFixture(
            data_root, persisted_save, persisted, error) &&
        persisted.quests().state(11) == 2 &&
        persisted.quests().state(12) == 1 &&
        persisted.retailSaveProgress().script_state_flags[23] == 1 &&
        osf::test::openNpcConversation(persisted, 0) &&
        persisted.conversationMessageId() == 1000013 &&
        persisted.groundItems().empty();
    if (persisted.conversationActive()) {
        persisted.advanceConversation();
    }
    std::filesystem::remove_all(fixture_root, cleanup_error);
    return check(
        no_repeat,
        "Saving the mission handoff repeated Kyle's briefing or reward.");
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "02100002")) {
        return 0;
    }
    return testForestOfClaws(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
