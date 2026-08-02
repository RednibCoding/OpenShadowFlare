#include "episode_one_test_support.hpp"
#include "items/item_audio.hpp"
#include "world/retail_save_file.hpp"
#include "world/retail_save_progress.hpp"
#include "world/retail_save_world_state.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

using osf::test::check;
using osf::test::containsSample;
using osf::test::loadSavedFixture;
using osf::test::raiseToLevel;

bool movePlayerNearObject(
    osf::WorldScene& world,
    std::int32_t character_number) {
    const auto object = std::find_if(
        world.scenarioObjects().begin(),
        world.scenarioObjects().end(),
        [character_number](const osf::ScenarioObjectActor& candidate) {
            return candidate.characterNumber() == character_number;
        });
    if (object == world.scenarioObjects().end()) {
        return false;
    }
    const osf::WorldPosition target = object->position();
    for (std::int32_t update = 0; update < 5000; ++update) {
        if (std::abs(world.playerWorldX() - target.x) <= 300 &&
            std::abs(world.playerWorldY() - target.y) <= 300) {
            world.cancelPlayerMovement();
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

osf::RetailSaveProgress remainsProgress(
    const osf::WorldScene& seed,
    std::int32_t remains_state,
    std::int32_t next_state = 0) {
    osf::RetailSaveProgress progress = seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    for (const std::int32_t completed : {0, 1, 2, 3, 4, 6, 7}) {
        progress.quest_flags[static_cast<std::size_t>(completed)] = 2;
    }
    progress.quest_flags[8] = remains_state;
    progress.quest_flags[9] = next_state;
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
        0x49,
        &error);
}

bool testRemainsMission(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "Remains";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The Remains fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerData level_fifty = seed.playerData();
    if (!check(
            raiseToLevel(level_fifty, 50, seed.parameterTables()),
            "The Remains fixture could not reach its authored area level.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_one_remains_test";
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
                level_fifty,
                remainsProgress(seed, 1),
                {true, 1000003, 1},
                error),
            "The active Remains route fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene room;
    if (!check(
            loadSavedFixture(data_root, route_save, room, error) &&
                room.scenarioId() == 1000003 &&
                room.scenario().title() == "Hanged Men's Forest" &&
                osf::test::walkThroughScenarioTrigger(
                    room, 1, 1040000) &&
                room.scenario().title() ==
                    "Remains of Reincarnation" &&
                room.transitionScenario(
                    {1040000, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    room, 1, 1040001) &&
                room.transitionScenario(
                    {1040001, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    room, 1, 1040002) &&
                room.retailSaveWorldState().entry_value == 0 &&
                room.transitionScenario(
                    {1040002, 2, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                movePlayerNearObject(room, 10021000),
            "The authored map edges did not reach the Remains clear room.")) {
        std::cerr << error << '\n';
        return false;
    }

    const auto enemyCount = [&room](const char* name) {
        return static_cast<std::int32_t>(std::count_if(
            room.enemies().begin(),
            room.enemies().end(),
            [name](const osf::EnemyActor& enemy) {
                return enemy.name() == name;
            }));
    };
    if (!check(
            room.scenarioId() == 1040002 &&
                room.enemies().size() == 7 &&
                enemyCount("Earth Golem") == 2 &&
                enemyCount("King Earth Goblin") == 2 &&
                enemyCount("Arc Goblin Shaman") == 3 &&
                osf::test::markScenarioEnemiesDefeated(room, 0, 6),
            "The authored Remains group-clear roster differs from retail.")) {
        return false;
    }

    std::vector<std::int32_t> completion_audio;
    for (std::int32_t update = 0;
         update < 400 && room.quests().state(8) != 2;
         ++update) {
        room.update();
        const std::vector<std::int32_t> samples =
            room.takeAudioSamples();
        completion_audio.insert(
            completion_audio.end(), samples.begin(), samples.end());
    }
    const auto room_reward_anchor = std::find_if(
        room.scenarioObjects().begin(),
        room.scenarioObjects().end(),
        [](const osf::ScenarioObjectActor& object) {
            return object.characterNumber() == 10021000;
        });
    const auto room_reward_item = std::find_if(
        room.groundItems().begin(),
        room.groundItems().end(),
        [&room, &room_reward_anchor](const osf::GroundItem& item) {
            return room_reward_anchor !=
                       room.scenarioObjects().end() &&
                   std::abs(
                       item.position.x -
                       room_reward_anchor->position().x) <= 400 &&
                   std::abs(
                       item.position.y -
                       room_reward_anchor->position().y) <= 400;
        });
    if (!check(
            room.quests().state(8) == 2 &&
                room.quests().lastCue() == osf::QuestCue::completed &&
                containsSample(completion_audio, 34) &&
                containsSample(completion_audio, 31) &&
                containsSample(completion_audio, 66) &&
                osf::test::scriptedObjectVisible(
                    room, 10011000, false) &&
                osf::test::scriptedObjectVisible(
                    room, 10011001, true) &&
                osf::test::scriptedObjectVisible(
                    room, 10011002, true) &&
                osf::test::scriptedObjectVisible(
                    room, 10021000, false) &&
                osf::test::scriptedObjectVisible(
                    room, 10021001, true) &&
                room_reward_anchor !=
                    room.scenarioObjects().end() &&
                room_reward_item != room.groundItems().end(),
            "The Remains clear did not apply its doors, sounds, loot, and cue.")) {
        std::cerr << "quest=" << room.quests().state(8)
                  << " samples="
                  << containsSample(completion_audio, 34) << ','
                  << containsSample(completion_audio, 31) << ','
                  << containsSample(completion_audio, 66)
                  << " doors="
                  << osf::test::scriptedObjectVisible(
                         room, 10011000, false) << ','
                  << osf::test::scriptedObjectVisible(
                         room, 10011001, true) << ','
                  << osf::test::scriptedObjectVisible(
                         room, 10011002, true) << ','
                  << osf::test::scriptedObjectVisible(
                         room, 10021000, false) << ','
                  << osf::test::scriptedObjectVisible(
                         room, 10021001, true)
                  << " anchor="
                  << (room_reward_anchor !=
                      room.scenarioObjects().end())
                  << " reward="
                  << (room_reward_item != room.groundItems().end())
                  << " items=" << room.groundItems().size() << '\n';
        if (room_reward_anchor != room.scenarioObjects().end()) {
            std::cerr << "anchor-position="
                      << room_reward_anchor->position().x << ','
                      << room_reward_anchor->position().y << '\n';
        }
        return false;
    }

    const osf::GroundItem room_reward = *room_reward_item;
    const osf::ItemDefinition* room_reward_definition =
        room.itemDatabase().find(
            room_reward.item.category,
            room_reward.item.definition_id);
    room.takeAudioSamples();
    for (std::int32_t update = 0; update < 19; ++update) {
        room.update();
    }
    if (!check(
            room_reward_definition &&
                containsSample(
                    room.takeAudioSamples(),
                    osf::retailItemLandingSound(
                        *room_reward_definition)),
            "The Remains room reward did not use its item landing sound.")) {
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
            "The completed Remains fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene returned;
    if (!check(
            loadSavedFixture(data_root, return_save, returned, error) &&
                osf::test::openNpcConversation(returned, 0) &&
                returned.conversationMessageId() == 1000022 &&
                returned.groundItems().size() == 1 &&
                returned.groundItems().front().item.category == 4 &&
                returned.groundItems().front().item.definition_id == 0 &&
                returned.groundItems().front().item.quantity == 6000,
            "Alex did not create the authored 6,000-Gold reward.")) {
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
            "Alex's Remains reward did not play the Gold landing sound.")) {
        return false;
    }

    returned.advanceConversation();
    if (!check(
            returned.conversationMessageId() == 1000023,
            "Alex skipped the road opened beyond the Remains.")) {
        return false;
    }
    returned.advanceConversation();
    const std::vector<std::int32_t> next_audio =
        returned.takeAudioSamples();
    if (!check(
            returned.conversationMessageId() == 1000024 &&
                returned.quests().state(9) == 1 &&
                returned.quests().notice().quest_id == 9 &&
                returned.quests().notice().counter == 600 &&
                containsSample(next_audio, 65),
            "Alex did not start the mission beyond the Remains.")) {
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
            "The Remains follow-up state could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene persisted;
    const bool completed =
        loadSavedFixture(
            data_root, completed_save, persisted, error) &&
        persisted.quests().state(8) == 2 &&
        persisted.quests().state(9) == 1 &&
        osf::test::openNpcConversation(persisted, 0) &&
        persisted.conversationMessageId() == 1000025 &&
        persisted.groundItems().empty();
    if (persisted.conversationActive()) {
        persisted.advanceConversation();
    }
    std::filesystem::remove_all(fixture_root, cleanup_error);
    return check(
        completed,
        "Saving and loading repeated Alex's Remains reward or handoff.");
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "01040002")) {
        return 0;
    }
    return testRemainsMission(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
