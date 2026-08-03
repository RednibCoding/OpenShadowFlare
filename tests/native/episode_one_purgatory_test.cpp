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

osf::RetailSaveProgress purgatoryProgress(
    const osf::WorldScene& seed,
    std::int32_t purgatory_state,
    std::int32_t remains_state = 0) {
    osf::RetailSaveProgress progress =
        seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    progress.quest_flags[0] = 2;
    progress.quest_flags[1] = 2;
    progress.quest_flags[2] = 2;
    progress.quest_flags[3] = 2;
    progress.quest_flags[4] = 2;
    progress.quest_flags[6] = 2;
    progress.quest_flags[7] = purgatory_state;
    progress.quest_flags[8] = remains_state;
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
        0x48,
        &error);
}

bool openAlexConversation(osf::WorldScene& world) {
    return osf::test::openNpcConversation(world, 0);
}

bool testPurgatoryMission(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "Purgatory";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The Purgatory fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerData level_forty = seed.playerData();
    if (!check(
            raiseToLevel(
                level_forty, 40, seed.parameterTables()),
            "The Purgatory fixture could not reach its authored area level.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_one_purgatory_test";
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
                level_forty,
                purgatoryProgress(seed, 1),
                {true, 1000002, 2},
                error),
            "The active Purgatory route fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene room;
    if (!check(
            loadSavedFixture(data_root, route_save, room, error) &&
                room.scenarioId() == 1000002 &&
                room.scenario().title() == "Vaporous Forest" &&
                osf::test::walkThroughScenarioTrigger(
                    room, 2, 1030000) &&
                room.scenario().title() ==
                    "Purgatory of Judgments" &&
                room.retailSaveWorldState().entry_value == 0 &&
                room.transitionScenario(
                    {1030000, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    room, 1, 1030002) &&
                room.retailSaveWorldState().entry_value == 0,
            "The authored map edges did not reach Purgatory's clear room.")) {
        std::cerr << error << '\n';
        return false;
    }

    const std::int32_t shamans =
        static_cast<std::int32_t>(std::count_if(
            room.enemies().begin(),
            room.enemies().end(),
            [](const osf::EnemyActor& enemy) {
                return enemy.name() == "Arc Shaman";
            }));
    const std::int32_t bats =
        static_cast<std::int32_t>(std::count_if(
            room.enemies().begin(),
            room.enemies().end(),
            [](const osf::EnemyActor& enemy) {
                return enemy.name() == "Arc Thunder Bat";
            }));
    if (!check(
            room.scenarioId() == 1030002 &&
                room.enemies().size() == 7 &&
                shamans == 3 && bats == 4 &&
                osf::test::markScenarioEnemiesDefeated(
                    room, 0, 6),
            "The authored Purgatory group-clear encounter differs from retail.")) {
        return false;
    }

    std::vector<std::int32_t> completion_audio;
    for (std::int32_t update = 0;
         update < 400 && room.quests().state(7) != 2;
         ++update) {
        room.update();
        const std::vector<std::int32_t> samples =
            room.takeAudioSamples();
        completion_audio.insert(
            completion_audio.end(),
            samples.begin(),
            samples.end());
    }
    if (!check(
            room.quests().state(7) == 2 &&
                room.quests().lastCue() ==
                    osf::QuestCue::completed &&
                containsSample(completion_audio, 66) &&
                osf::test::scriptedObjectVisible(
                    room, 10011000, false) &&
                osf::test::scriptedObjectVisible(
                    room, 10011001, true) &&
                osf::test::scriptedObjectVisible(
                    room, 10011002, true),
            "Purgatory did not complete after all seven death fades.")) {
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
            "The completed Purgatory fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene returned;
    if (!check(
            loadSavedFixture(
                data_root, return_save, returned, error) &&
                openAlexConversation(returned) &&
                returned.conversationMessageId() == 1000017 &&
                returned.groundItems().size() == 1 &&
                returned.groundItems().front().item.category == 4 &&
                returned.groundItems().front().item.definition_id == 0 &&
                returned.groundItems().front().item.quantity == 4000,
            "Alex did not create the authored 4,000-Gold reward.")) {
        std::cerr << "message="
                  << returned.conversationMessageId()
                  << " items=" << returned.groundItems().size()
                  << " q7=" << returned.quests().state(7)
                  << " q8=" << returned.quests().state(8)
                  << '\n';
        return false;
    }
    returned.takeAudioSamples();
    for (std::int32_t update = 0; update < 19; ++update) {
        returned.update();
    }
    if (!check(
            containsSample(returned.takeAudioSamples(), 85),
            "Alex's Purgatory reward did not play the Gold landing sound.")) {
        return false;
    }

    for (const std::int32_t message : {1000018, 1000019}) {
        returned.advanceConversation();
        if (!check(
                returned.conversationMessageId() == message,
                "Alex's Remains briefing skipped a message.")) {
            return false;
        }
    }
    returned.advanceConversation();
    const std::vector<std::int32_t> next_audio =
        returned.takeAudioSamples();
    if (!check(
            returned.conversationMessageId() == 1000020 &&
                returned.quests().state(8) == 1 &&
                returned.quests().notice().quest_id == 8 &&
                returned.quests().notice().counter == 600 &&
                containsSample(next_audio, 65),
            "Alex did not start the Remains of Reincarnation mission.")) {
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
            "The Purgatory and Remains states could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene persisted;
    const bool completed =
        loadSavedFixture(
            data_root, completed_save, persisted, error) &&
        persisted.quests().state(7) == 2 &&
        persisted.quests().state(8) == 1 &&
        openAlexConversation(persisted) &&
        persisted.conversationMessageId() == 1000021 &&
        persisted.groundItems().empty();
    if (persisted.conversationActive()) {
        persisted.advanceConversation();
    }
    std::filesystem::remove_all(fixture_root, cleanup_error);
    return check(
        completed,
        "Saving and loading repeated Alex's Purgatory reward or handoff.");
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "01030002")) {
        return 0;
    }
    return testPurgatoryMission(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
