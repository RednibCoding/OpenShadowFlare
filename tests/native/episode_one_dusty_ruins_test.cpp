#include "world/enemy_actor.hpp"
#include "world/player_data.hpp"
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

constexpr std::int32_t kRetailDeathPresentationAction = 11;

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool containsSample(
    const std::vector<std::int32_t>& samples,
    std::int32_t sample) {
    return std::find(samples.begin(), samples.end(), sample) !=
           samples.end();
}

bool findNpcPointerPoint(
    osf::WorldScene& world,
    std::int32_t npc_id,
    osf::ScreenPosition& point) {
    const auto found = std::find_if(
        world.npcs().begin(),
        world.npcs().end(),
        [npc_id](const osf::NpcActor& npc) {
            return npc.id() == npc_id;
        });
    if (found == world.npcs().end()) {
        return false;
    }
    const osf::ScreenPosition anchor =
        osf::calculateRealPosition(found->position());
    for (std::int32_t y = -found->labelHeight(); y <= 16; ++y) {
        for (std::int32_t x = -48; x <= 48; ++x) {
            point = {
                anchor.x - world.cameraScreenX() + x,
                anchor.y - world.cameraScreenY() + y,
            };
            if (point.x < 0 || point.x >= 640 ||
                point.y < 0 || point.y >= 480) {
                continue;
            }
            world.updatePointerHover(point.x, point.y);
            if (world.hoveredNpcId() == npc_id) {
                return true;
            }
        }
    }
    return false;
}

bool updateUntilConversation(
    osf::WorldScene& world,
    std::vector<std::int32_t>* audio = nullptr) {
    for (std::int32_t update = 0;
         update < 2000 && !world.conversationActive();
         ++update) {
        world.update();
        std::vector<std::int32_t> samples =
            world.takeAudioSamples();
        if (audio) {
            audio->insert(
                audio->end(), samples.begin(), samples.end());
        }
    }
    return world.conversationActive();
}

bool openOstareConversation(
    osf::WorldScene& world,
    std::vector<std::int32_t>* audio = nullptr) {
    osf::ScreenPosition pointer;
    for (std::int32_t update = 0; update < 2000; ++update) {
        if (findNpcPointerPoint(world, 0, pointer)) {
            world.cancelPlayerMovement();
            return world.commandWorldInteraction(
                       pointer.x, pointer.y) &&
                   updateUntilConversation(world, audio);
        }
        const auto ostare = std::find_if(
            world.npcs().begin(),
            world.npcs().end(),
            [](const osf::NpcActor& npc) {
                return npc.id() == 0;
            });
        if (ostare == world.npcs().end()) {
            return false;
        }
        if (update % 30 == 0) {
            const osf::ScreenPosition target =
                osf::calculateRealPosition(ostare->position());
            world.commandPlayerMovement(
                target.x - world.cameraScreenX(),
                target.y - world.cameraScreenY());
        }
        world.update();
        std::vector<std::int32_t> samples =
            world.takeAudioSamples();
        if (audio) {
            audio->insert(
                audio->end(), samples.begin(), samples.end());
        }
    }
    return false;
}

bool raiseToLevel(
    osf::PlayerData& player,
    std::int32_t level,
    const osf::TableDatabase& tables) {
    while (player.level() < level) {
        const std::int32_t threshold =
            player.experienceThreshold(tables);
        if (threshold <= player.experience()) {
            return false;
        }
        player.addExperience(threshold - player.experience());
        if (!player.applyLevelThreshold(tables)) {
            return false;
        }
    }
    return player.level() == level;
}

osf::RetailSaveProgress dustyRuinsProgress(
    const osf::WorldScene& seed,
    std::int32_t quest_state) {
    osf::RetailSaveProgress progress =
        seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    progress.quest_flags[0] = 2;
    progress.quest_flags[3] = quest_state;
    if (progress.script_state_flags.size() < 72) {
        progress.script_state_flags.resize(72, 0);
    }
    progress.script_state_flags[4] = 1;
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
        0x42,
        &error);
}

bool loadFixture(
    const std::filesystem::path& data_root,
    const std::filesystem::path& save_path,
    osf::WorldScene& world,
    std::string& error) {
    osf::PlayerLoadRequest request;
    request.source = osf::PlayerDataSource::retail_save;
    request.save_path = save_path;
    return world.loadInitialScenario(data_root, request, &error);
}

bool markJudgementEnemiesDefeated(osf::WorldScene& world) {
    std::vector<osf::EnemyActor>& enemies =
        const_cast<std::vector<osf::EnemyActor>&>(world.enemies());
    std::int32_t defeated = 0;
    for (osf::EnemyActor& enemy : enemies) {
        if (enemy.id() < 0 || enemy.id() > 7) {
            continue;
        }
        osf::EnemyDamageReceiverState state =
            enemy.damageReceiverState(world.scenarioId());
        state.current_life = 0;
        state.presentation_action = kRetailDeathPresentationAction;
        state.presentation_counter = 0;
        state.action_lock = 1;
        enemy.applyDamageReceiverState(state);
        ++defeated;
    }
    return defeated == 8;
}

bool scriptedObjectState(
    const osf::WorldScene& world,
    std::int32_t character_number,
    bool visible) {
    const auto found = std::find_if(
        world.scenarioObjects().begin(),
        world.scenarioObjects().end(),
        [character_number](const osf::ScenarioObjectActor& object) {
            return object.characterNumber() == character_number;
        });
    return found != world.scenarioObjects().end() &&
           found->visible() == visible;
}

bool testDustyRuinsMission(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "DustyRuins";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The Dusty Ruins fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerData level_thirty = seed.playerData();
    if (!check(
            raiseToLevel(
                level_thirty, 30, seed.parameterTables()),
            "The Dusty Ruins fixture could not reach the retail level gate.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_dusty_ruins_quest_test";
    const std::filesystem::path offer_save =
        fixture_root / "offer" / "Save" / "0000.Ssv";
    const std::filesystem::path room_save =
        fixture_root / "room" / "Save" / "0000.Ssv";
    const std::filesystem::path return_save =
        fixture_root / "return" / "Save" / "0000.Ssv";
    const std::filesystem::path rewarded_save =
        fixture_root / "rewarded" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);

    if (!check(
            writeFixture(
                offer_save,
                seed,
                level_thirty,
                dustyRuinsProgress(seed, 0),
                {false, 0, 0},
                error),
            "The Dusty Ruins offer fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene offered;
    if (!check(
            loadFixture(data_root, offer_save, offered, error) &&
                openOstareConversation(offered) &&
                offered.conversationMessageId() == 1000007,
            "Ostare did not announce the authored Dusty Ruins lead.")) {
        std::cerr << error << '\n';
        return false;
    }
    offered.advanceConversation();
    const std::vector<std::int32_t> offer_audio =
        offered.takeAudioSamples();
    if (!check(
            offered.conversationMessageId() == 1000008 &&
                offered.quests().state(3) == 1 &&
                offered.quests().lastCue() ==
                    osf::QuestCue::updated &&
                offered.quests().notice().quest_id == 3 &&
                offered.quests().notice().counter == 600 &&
                containsSample(offer_audio, 65),
            "Ostare did not start mission three with its retail notice and cue.")) {
        return false;
    }
    offered.advanceConversation();
    if (!check(
            offered.conversationMessageId() == 1000009,
            "Ostare skipped the Dusty Ruins Warp Gate advice.")) {
        return false;
    }
    offered.advanceConversation();
    if (!check(
            !offered.conversationActive(),
            "The Dusty Ruins offer did not release Ostare.")) {
        return false;
    }

    if (!check(
            writeFixture(
                room_save,
                offered,
                offered.playerData(),
                offered.retailSaveProgress(),
                {true, 10004, 0},
                error),
            "The Room of Judgment fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene room;
    if (!check(
            loadFixture(data_root, room_save, room, error) &&
                room.scenarioId() == 10004 &&
                room.quests().state(3) == 1 &&
                markJudgementEnemiesDefeated(room),
            "The Room of Judgment encounter could not be prepared.")) {
        std::cerr << error << '\n';
        return false;
    }
    std::vector<std::int32_t> completion_audio;
    for (std::int32_t update = 0;
         update < 400 && room.quests().state(3) != 2;
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
            room.quests().state(3) == 2 &&
                room.quests().lastCue() ==
                    osf::QuestCue::completed &&
                containsSample(completion_audio, 66) &&
                scriptedObjectState(room, 10011000, false) &&
                scriptedObjectState(room, 10011001, true) &&
                scriptedObjectState(room, 10011002, true),
            "The eight defeated slots did not complete mission three and "
            "apply the authored room state.")) {
        std::cerr << "quest=" << room.quests().state(3)
                  << " cue="
                  << static_cast<std::int32_t>(
                         room.quests().lastCue())
                  << " sample66="
                  << containsSample(completion_audio, 66)
                  << " objects="
                  << scriptedObjectState(room, 10011000, false)
                  << ','
                  << scriptedObjectState(room, 10011001, true)
                  << ','
                  << scriptedObjectState(room, 10011002, true)
                  << '\n';
        for (const osf::EnemyActor& enemy : room.enemies()) {
            if (enemy.id() >= 0 && enemy.id() <= 7) {
                std::cerr << "enemy " << enemy.id()
                          << " life=" << enemy.currentLife()
                          << " expired=" << enemy.expired()
                          << " chart=" << enemy.animationChart()
                          << " frame=" << enemy.animationFrame()
                          << '\n';
            }
        }
        return false;
    }

    if (!check(
            writeFixture(
                return_save,
                room,
                room.playerData(),
                room.retailSaveProgress(),
                {false, 0, 0},
                error),
            "The completed Dusty Ruins fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene returned;
    if (!check(
            loadFixture(data_root, return_save, returned, error) &&
                returned.quests().state(3) == 2 &&
                openOstareConversation(returned) &&
                returned.conversationMessageId() == 1000011,
            "Ostare did not recognize the completed Dusty Ruins mission.")) {
        std::cerr << error << '\n';
        return false;
    }
    const std::size_t reward_count =
        returned.groundItems().size();
    const osf::RetailSaveProgress reward_progress =
        returned.retailSaveProgress();
    if (!check(
            reward_count == 1 &&
                reward_progress.script_state_flags.size() > 2 &&
                reward_progress.script_state_flags[2] == 1,
            "Ostare did not create and latch his authored mission reward.")) {
        return false;
    }
    returned.advanceConversation();
    if (!check(
            returned.conversationMessageId() == 1000012,
            "Ostare skipped the Cold Svalt assignment after the reward.")) {
        return false;
    }
    returned.advanceConversation();
    if (!check(
            !returned.conversationActive(),
            "Ostare did not release the completed mission conversation.")) {
        return false;
    }

    if (!check(
            writeFixture(
                rewarded_save,
                returned,
                returned.playerData(),
                returned.retailSaveProgress(),
                {false, 0, 0},
                error),
            "The latched Dusty Ruins reward could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene persisted;
    const bool reward_persisted =
        loadFixture(data_root, rewarded_save, persisted, error) &&
        persisted.quests().state(3) == 2 &&
        persisted.retailSaveProgress().script_state_flags.size() > 2 &&
        persisted.retailSaveProgress().script_state_flags[2] == 1 &&
        openOstareConversation(persisted) &&
        persisted.conversationMessageId() == 1000012 &&
        persisted.groundItems().empty();
    if (persisted.conversationActive()) {
        persisted.advanceConversation();
    }
    if (!check(
            reward_persisted,
            "Saving and loading repeated Ostare's Dusty Ruins reward.")) {
        std::cerr << error << '\n';
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
            data_root / "Scenario" / "00010004")) {
        return 0;
    }
    return testDustyRuinsMission(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
