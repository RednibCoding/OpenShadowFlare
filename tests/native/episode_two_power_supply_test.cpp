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

osf::RetailSaveProgress powerSupplyProgress(
    const osf::WorldScene& seed) {
    osf::RetailSaveProgress progress = seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    for (const std::int32_t completed :
         {0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 13, 14, 15}) {
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
        0x56,
        &error);
}

std::int32_t groundGold(const osf::WorldScene& world) {
    std::int32_t gold = 0;
    for (const osf::GroundItem& item : world.groundItems()) {
        if (item.item.category == 4 && item.item.definition_id == 0) {
            gold += item.item.quantity;
        }
    }
    return gold;
}

bool waitForMissionCompletion(
    osf::WorldScene& world,
    std::vector<std::int32_t>& audio,
    std::int32_t maximum_updates = 800) {
    for (std::int32_t update = 0;
         update < maximum_updates && world.quests().state(16) != 2;
         ++update) {
        world.update();
        const std::vector<std::int32_t> samples =
            world.takeAudioSamples();
        audio.insert(audio.end(), samples.begin(), samples.end());
    }
    return world.quests().state(16) == 2;
}

bool testPowerSupply(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "PowerSupply";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The power-supply fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerData level_sixty = seed.playerData();
    if (!check(
            raiseToLevel(level_sixty, 60, seed.parameterTables()),
            "The power-supply fixture could not reach its area level.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_two_power_supply_test";
    const std::filesystem::path warning_save =
        fixture_root / "warning" / "Save" / "0000.Ssv";
    const std::filesystem::path briefed_save =
        fixture_root / "briefed" / "Save" / "0000.Ssv";
    const std::filesystem::path completed_save =
        fixture_root / "completed" / "Save" / "0000.Ssv";
    const std::filesystem::path dragon_save =
        fixture_root / "dragon" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);

    if (!check(
            writeFixture(
                warning_save,
                seed,
                level_sixty,
                powerSupplyProgress(seed),
                {true, 2200000, 0},
                error),
            "Kirarru's dragon warning fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene fanann;
    std::vector<std::int32_t> briefing_audio;
    if (!check(
            loadSavedFixture(data_root, warning_save, fanann, error) &&
                fanann.quests().state(16) == 0 &&
                fanann.quests().state(17) == 0 &&
                osf::test::openNpcConversation(
                    fanann, 0, &briefing_audio) &&
                fanann.conversationMessageId() == 1000007,
            "Lytle did not respond to Kirarru's dragon warning.")) {
        std::cerr << error << " message="
                  << fanann.conversationMessageId() << '\n';
        return false;
    }
    for (const std::int32_t message : {1000008, 1000009}) {
        fanann.advanceConversation();
        if (!check(
                fanann.conversationMessageId() == message,
                "Lytle's power-supply briefing skipped a message.")) {
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
                fanann.quests().state(16) == 1 &&
                fanann.quests().state(17) == 0 &&
                containsSample(briefing_audio, 65),
            "Lytle did not start mission sixteen once.")) {
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
            "Lytle's power-supply briefing could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene route;
    std::vector<std::int32_t> repeat_audio;
    if (!check(
            loadSavedFixture(data_root, briefed_save, route, error) &&
                route.quests().state(16) == 1 &&
                osf::test::openNpcConversation(
                    route, 0, &repeat_audio) &&
                route.conversationMessageId() == 1000010 &&
                !containsSample(repeat_audio, 65),
            "Saving mission sixteen replayed its briefing or update.")) {
        std::cerr << error << " message="
                  << route.conversationMessageId() << '\n';
        return false;
    }
    route.advanceConversation();
    route.advanceConversation();

    if (!check(
            route.transitionScenario(
                {2200000, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 1, 2200001) &&
                route.scenario().title() == "Butterfly Hill" &&
                route.transitionScenario(
                    {2200001, 2, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 2, 2200004) &&
                route.scenario().title() == "Labyrinth of Mauve " &&
                route.transitionScenario(
                    {2200004, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 1, 2200005) &&
                route.scenario().title() ==
                    "Near The Power Supply Facility" &&
                route.transitionScenario(
                    {2200005, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 1, 2230000) &&
                route.scenario().title() ==
                    "Fort of the Power Supply",
            "The authored route did not reach the Power Supply Fort.")) {
        std::cerr << error << " scenario=" << route.scenarioId()
                  << " title=" << route.scenario().title() << '\n';
        return false;
    }

    const auto crimson_sword = std::find_if(
        route.enemies().begin(),
        route.enemies().end(),
        [](const osf::EnemyActor& enemy) {
            return enemy.id() == 10000 &&
                   enemy.name() == "Crimson Sword";
        });
    if (!check(
            crimson_sword != route.enemies().end() &&
                route.quests().state(16) == 1 &&
                osf::test::markScenarioEnemiesDefeated(
                    route, 10000, 10000),
            "The authored Crimson Sword objective could not be defeated.")) {
        return false;
    }
    std::vector<std::int32_t> completion_audio;
    if (!check(
            waitForMissionCompletion(route, completion_audio) &&
                route.quests().lastCue() ==
                    osf::QuestCue::completed &&
                containsSample(completion_audio, 66) &&
                route.quests().state(17) == 0,
            "Crimson Sword's completed death did not finish mission sixteen.")) {
        return false;
    }

    if (!check(
            osf::test::walkThroughScenarioTrigger(
                route, 0, 2200005) &&
                route.retailSaveWorldState().entry_value == 1,
            "The Power Supply Fort did not return to its approach map.")) {
        return false;
    }
    if (!check(
            writeFixture(
                completed_save,
                route,
                route.playerData(),
                route.retailSaveProgress(),
                {true, 2200000, 0},
                error),
            "The completed power-supply mission could not return to Lytle.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene rewarded;
    if (!check(
            loadSavedFixture(data_root, completed_save, rewarded, error) &&
                osf::test::openNpcConversation(rewarded, 0) &&
                rewarded.conversationMessageId() == 1000011 &&
                rewarded.groundItems().size() == 4 &&
                groundGold(rewarded) == 40000,
            "Lytle did not drop the authored 40,000-Gold reward.")) {
        std::cerr << error << " message="
                  << rewarded.conversationMessageId()
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
                reward_audio.begin(), reward_audio.end(), 85) == 4,
            "Lytle's Gold stacks did not play four landing sounds.")) {
        return false;
    }

    for (std::int32_t message = 1000012;
         message <= 1000016;
         ++message) {
        rewarded.advanceConversation();
        if (!check(
                rewarded.conversationMessageId() == message,
                "Lytle's dragon briefing skipped a message.")) {
            std::cerr << "message="
                      << rewarded.conversationMessageId()
                      << " expected=" << message << '\n';
            return false;
        }
    }
    rewarded.advanceConversation();
    rewarded.advanceConversation();
    const std::vector<std::int32_t> dragon_audio =
        rewarded.takeAudioSamples();
    if (!check(
            !rewarded.conversationActive() &&
                rewarded.quests().state(16) == 2 &&
                rewarded.quests().state(17) == 1 &&
                containsSample(dragon_audio, 65),
            "Lytle did not start the dragon mission after the reward.")) {
        return false;
    }

    if (!check(
            writeFixture(
                dragon_save,
                rewarded,
                rewarded.playerData(),
                rewarded.retailSaveProgress(),
                rewarded.retailSaveWorldState(),
                error),
            "Lytle's dragon briefing could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene persisted;
    std::vector<std::int32_t> persisted_audio;
    if (!check(
            loadSavedFixture(data_root, dragon_save, persisted, error) &&
                persisted.quests().state(16) == 2 &&
                persisted.quests().state(17) == 1 &&
                persisted.groundItems().empty() &&
                osf::test::openNpcConversation(
                    persisted, 0, &persisted_audio) &&
                persisted.conversationMessageId() == 1000017 &&
                !containsSample(persisted_audio, 65),
            "Saving the dragon briefing replayed its reward or update.")) {
        std::cerr << error << " message="
                  << persisted.conversationMessageId() << '\n';
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
            data_root / "Scenario" / "02230000")) {
        return 0;
    }
    return testPowerSupply(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
