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

bool openAlexConversation(
    osf::WorldScene& world,
    std::vector<std::int32_t>* audio = nullptr) {
    return osf::test::openNpcConversation(world, 0, audio);
}

osf::RetailSaveProgress coldRuinsProgress(
    const osf::WorldScene& seed,
    std::int32_t cold_ruins_state,
    std::int32_t purgatory_state = 0) {
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
    progress.quest_flags[6] = cold_ruins_state;
    progress.quest_flags[7] = purgatory_state;
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
        0x47,
        &error);
}

bool testColdRuinsMission(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "ColdRuins";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The Cold Ruins fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerData level_thirty = seed.playerData();
    if (!check(
            raiseToLevel(
                level_thirty, 30, seed.parameterTables()),
            "The Cold Ruins fixture could not reach Cold Svalt level.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_one_cold_ruins_test";
    const std::filesystem::path offer_save =
        fixture_root / "offer" / "Save" / "0000.Ssv";
    const std::filesystem::path room_save =
        fixture_root / "room" / "Save" / "0000.Ssv";
    const std::filesystem::path return_save =
        fixture_root / "return" / "Save" / "0000.Ssv";
    const std::filesystem::path completed_save =
        fixture_root / "completed" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);

    if (!check(
            writeFixture(
                offer_save,
                seed,
                level_thirty,
                coldRuinsProgress(seed, 0),
                {true, 1000000, 0},
                error),
            "The Cold Ruins offer fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene offered;
    if (!check(
            loadSavedFixture(
                data_root, offer_save, offered, error) &&
                openAlexConversation(offered) &&
                offered.conversationMessageId() == 1000009,
            "Alex did not begin the authored Cold Ruins briefing.")) {
        std::cerr << error << '\n';
        return false;
    }
    for (const std::int32_t message : {1000010, 1000011}) {
        offered.advanceConversation();
        if (!check(
                offered.conversationMessageId() == message,
                "Alex's Cold Ruins briefing skipped a message.")) {
            return false;
        }
    }
    offered.advanceConversation();
    const std::vector<std::int32_t> offer_audio =
        offered.takeAudioSamples();
    if (!check(
            offered.conversationMessageId() == 1000012 &&
                offered.quests().state(6) == 1 &&
                offered.quests().lastCue() ==
                    osf::QuestCue::updated &&
                offered.quests().notice().quest_id == 6 &&
                offered.quests().notice().counter == 600 &&
                containsSample(offer_audio, 65),
            "Alex did not start mission six with its notice and cue.")) {
        return false;
    }
    offered.advanceConversation();
    if (!check(
            !offered.conversationActive(),
            "The Cold Ruins briefing did not release Alex.")) {
        return false;
    }

    if (!check(
            writeFixture(
                room_save,
                offered,
                offered.playerData(),
                offered.retailSaveProgress(),
                {true, 1020002, 0},
                error),
            "The Cold Ruins bottom-floor fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene room;
    if (!check(
            loadSavedFixture(data_root, room_save, room, error) &&
                room.scenarioId() == 1020002 &&
                room.scenario().title() == "Cold Ruins" &&
                room.enemies().size() == 7 &&
                osf::test::markScenarioEnemiesDefeated(
                    room, 0, 6),
            "The Cold Ruins group-clear encounter could not be prepared.")) {
        std::cerr << error << '\n';
        return false;
    }
    std::vector<std::int32_t> completion_audio;
    for (std::int32_t update = 0;
         update < 400 && room.quests().state(6) != 2;
         ++update) {
        room.update();
        const std::vector<std::int32_t> samples =
            room.takeAudioSamples();
        completion_audio.insert(
            completion_audio.end(), samples.begin(), samples.end());
    }
    if (!check(
            room.quests().state(6) == 2 &&
                room.quests().lastCue() ==
                    osf::QuestCue::completed &&
                containsSample(completion_audio, 66) &&
                osf::test::scriptedObjectVisible(
                    room, 10011000, false) &&
                osf::test::scriptedObjectVisible(
                    room, 10011001, true) &&
                osf::test::scriptedObjectVisible(
                    room, 10011002, true),
            "The seven defeated Cold Ruins slots did not complete mission six.")) {
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
            "The completed Cold Ruins fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene returned;
    if (!check(
            loadSavedFixture(
                data_root, return_save, returned, error) &&
                openAlexConversation(returned) &&
                returned.conversationMessageId() == 1000014 &&
                returned.groundItems().size() == 1 &&
                returned.groundItems().front().item.category == 4 &&
                returned.groundItems().front().item.definition_id == 0 &&
                returned.groundItems().front().item.quantity == 2000,
            "Alex did not create the authored 2,000-Gold reward.")) {
        std::cerr << error << '\n';
        return false;
    }

    returned.takeAudioSamples();
    for (std::int32_t update = 0; update < 19; ++update) {
        returned.update();
    }
    if (!check(
            containsSample(returned.takeAudioSamples(), 85),
            "Alex's Gold reward did not play its authored landing sound.")) {
        return false;
    }
    returned.advanceConversation();
    const std::vector<std::int32_t> next_audio =
        returned.takeAudioSamples();
    if (!check(
            returned.conversationMessageId() == 1000015 &&
                returned.quests().state(7) == 1 &&
                returned.quests().notice().quest_id == 7 &&
                containsSample(next_audio, 65),
            "Alex did not hand off directly to the Purgatory mission.")) {
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
            "The Cold Ruins and Purgatory states could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene persisted;
    const bool completed =
        loadSavedFixture(
            data_root, completed_save, persisted, error) &&
        persisted.quests().state(6) == 2 &&
        persisted.quests().state(7) == 1 &&
        openAlexConversation(persisted) &&
        persisted.conversationMessageId() == 1000016 &&
        persisted.groundItems().empty();
    if (persisted.conversationActive()) {
        persisted.advanceConversation();
    }
    std::filesystem::remove_all(fixture_root, cleanup_error);
    return check(
        completed,
        "Saving and loading repeated Alex's Cold Ruins reward or handoff.");
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "01020002")) {
        return 0;
    }
    return testColdRuinsMission(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
