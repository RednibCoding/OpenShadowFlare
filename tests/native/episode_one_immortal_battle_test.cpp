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

osf::RetailSaveProgress battleProgress(
    const osf::WorldScene& seed) {
    osf::RetailSaveProgress progress = seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    for (const std::int32_t completed :
         {0, 1, 2, 3, 4, 6, 7, 8, 9}) {
        progress.quest_flags[
            static_cast<std::size_t>(completed)] = 2;
    }
    progress.quest_flags[10] = 1;
    if (progress.script_state_flags.size() < 72) {
        progress.script_state_flags.resize(72, 0);
    }
    progress.script_state_flags[11] = 1;
    progress.script_state_flags[15] = 1;
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
        0x4b,
        &error);
}

bool testImmortalBattle(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "ImmortalBattle";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The Immortal battle fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerData level_sixty = seed.playerData();
    if (!check(
            raiseToLevel(level_sixty, 60, seed.parameterTables()),
            "The Immortal battle fixture could not reach its area level.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_one_immortal_battle_test";
    const std::filesystem::path route_save =
        fixture_root / "route" / "Save" / "0000.Ssv";
    const std::filesystem::path return_save =
        fixture_root / "return" / "Save" / "0000.Ssv";
    const std::filesystem::path completed_save =
        fixture_root / "completed" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);

    if (!check(
            writeFixture(
                route_save,
                seed,
                level_sixty,
                battleProgress(seed),
                {true, 1050000, 0},
                error),
            "The active Immortal battle fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene room;
    if (!check(
            loadSavedFixture(data_root, route_save, room, error) &&
                room.scenarioId() == 1050000 &&
                room.transitionScenario(
                    {1050000, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    room, 1, 1050001) &&
                room.retailSaveWorldState().entry_value == 0 &&
                room.transitionScenario(
                    {1050001, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    room, 1, 1050002) &&
                room.retailSaveWorldState().entry_value == 0 &&
                room.transitionScenario(
                    {1050002, 2, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated,
            "The authored Immortal Remains doors did not reach the Gargoyle room.")) {
        std::cerr << error << '\n';
        return false;
    }

    const std::int32_t gargoyles =
        static_cast<std::int32_t>(std::count_if(
            room.enemies().begin(),
            room.enemies().end(),
            [](const osf::EnemyActor& enemy) {
                return enemy.name() == "Gargoyle" &&
                       enemy.lootTableRow() == 55;
            }));
    const std::int32_t ordinary_gold =
        static_cast<std::int32_t>(std::count_if(
            room.enemies().begin(),
            room.enemies().end(),
            [](const osf::EnemyActor& enemy) {
                return enemy.goldDropChance() == 50 &&
                       enemy.goldMinimum() == 200 &&
                       enemy.goldMaximum() == 300;
            }));
    const std::int32_t magic_gold =
        static_cast<std::int32_t>(std::count_if(
            room.enemies().begin(),
            room.enemies().end(),
            [](const osf::EnemyActor& enemy) {
                return enemy.goldDropChance() == 100 &&
                       enemy.goldMinimum() == 600 &&
                       enemy.goldMaximum() == 800;
            }));
    if (!check(
            room.enemies().size() == 7 &&
                gargoyles == 7 &&
                ordinary_gold == 4 &&
                magic_gold == 3 &&
                osf::test::markScenarioEnemiesDefeated(room, 0, 6),
            "The authored Immortal Remains Gargoyle group differs from retail.")) {
        std::cerr << "enemies=" << room.enemies().size()
                  << " gargoyles=" << gargoyles
                  << " gold=" << ordinary_gold << ',' << magic_gold
                  << '\n';
        for (const osf::EnemyActor& enemy : room.enemies()) {
            std::cerr << enemy.id() << ':' << enemy.name()
                      << " loot=" << enemy.lootTableRow()
                      << " gold=" << enemy.goldDropChance() << '/'
                      << enemy.goldMinimum() << '/'
                      << enemy.goldMaximum() << '\n';
        }
        return false;
    }

    std::vector<std::int32_t> completion_audio;
    for (std::int32_t update = 0;
         update < 400 && room.quests().state(10) != 2;
         ++update) {
        room.update();
        const std::vector<std::int32_t> samples =
            room.takeAudioSamples();
        completion_audio.insert(
            completion_audio.end(), samples.begin(), samples.end());
    }
    if (!check(
            room.quests().state(10) == 2 &&
                room.quests().lastCue() == osf::QuestCue::completed &&
                containsSample(completion_audio, 34) &&
                containsSample(completion_audio, 66) &&
                osf::test::scriptedObjectVisible(
                    room, 10011000, false) &&
                osf::test::scriptedObjectVisible(
                    room, 10011001, true) &&
                osf::test::scriptedObjectVisible(
                    room, 10011002, true),
            "The Gargoyle clear did not open its room and complete mission ten.")) {
        std::cerr << "quest=" << room.quests().state(10)
                  << " audio="
                  << containsSample(completion_audio, 34) << ','
                  << containsSample(completion_audio, 66)
                  << " doors="
                  << osf::test::scriptedObjectVisible(
                         room, 10011000, false) << ','
                  << osf::test::scriptedObjectVisible(
                         room, 10011001, true) << ','
                  << osf::test::scriptedObjectVisible(
                         room, 10011002, true) << '\n';
        return false;
    }

    if (!check(
            writeFixture(
                return_save,
                room,
                room.playerData(),
                room.retailSaveProgress(),
                {true, 1000000, 0},
                error),
            "The completed Gargoyle fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene returned;
    if (!check(
            loadSavedFixture(data_root, return_save, returned, error) &&
                osf::test::openNpcConversation(returned, 0) &&
                returned.conversationMessageId() == 1000030 &&
                returned.groundItems().size() == 1 &&
                returned.groundItems().front().item.category == 4 &&
                returned.groundItems().front().item.definition_id == 0 &&
                returned.groundItems().front().item.quantity == 10000,
            "Alex did not create the authored 10,000-Gold reward.")) {
        std::cerr << "message=" << returned.conversationMessageId()
                  << " items=" << returned.groundItems().size() << '\n';
        return false;
    }
    returned.takeAudioSamples();
    for (std::int32_t update = 0; update < 19; ++update) {
        returned.update();
    }
    if (!check(
            containsSample(returned.takeAudioSamples(), 85),
            "Alex's final Episode 1 reward did not play the Gold landing sound.")) {
        return false;
    }

    returned.advanceConversation();
    if (!check(
            returned.conversationMessageId() == 1000031,
            "Alex skipped the post-Gargoyle Tower of Ordeal message.")) {
        return false;
    }
    returned.advanceConversation();
    if (!check(
            !returned.conversationActive() &&
                returned.scenarioVisualActive() &&
                returned.scenarioVisual().visualId() == 0 &&
                !returned.scenarioVisualPatterns().patterns().empty(),
            "Alex did not launch the authored Episode 1 Epilogue.")) {
        return false;
    }

    for (std::int32_t frame = 0; frame < 300; ++frame) {
        returned.advanceScenarioVisualFrame();
    }
    returned.requestScenarioVisualAdvance();
    returned.advanceScenarioVisualFrame();
    returned.advanceScenarioVisualFrame();
    const std::size_t reward_count = returned.groundItems().size();
    if (!check(
            !returned.scenarioVisualActive() &&
                osf::test::openNpcConversation(returned, 0) &&
                returned.conversationMessageId() == 1000033 &&
                returned.groundItems().size() == reward_count,
            "The post-Epilogue Alex branch repeated the reward or skipped Mining Town.")) {
        return false;
    }
    returned.advanceConversation();

    if (!check(
            writeFixture(
                completed_save,
                returned,
                returned.playerData(),
                returned.retailSaveProgress(),
                {true, 1000000, 0},
                error),
            "The Episode 1 completion state could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene persisted;
    const bool completed =
        loadSavedFixture(
            data_root, completed_save, persisted, error) &&
        persisted.quests().state(10) == 2 &&
        persisted.retailSaveProgress().script_state_flags.size() > 71 &&
        persisted.retailSaveProgress().script_state_flags[11] == 2 &&
        persisted.retailSaveProgress().script_state_flags[71] == 1 &&
        osf::test::openNpcConversation(persisted, 0) &&
        persisted.conversationMessageId() == 1000033 &&
        persisted.groundItems().empty();
    if (persisted.conversationActive()) {
        persisted.advanceConversation();
    }
    std::filesystem::remove_all(fixture_root, cleanup_error);
    return check(
        completed,
        "Saving and loading repeated Alex's final reward or Epilogue handoff.");
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "01050002")) {
        return 0;
    }
    return testImmortalBattle(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
