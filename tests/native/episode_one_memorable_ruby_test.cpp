#include "core/retail_random.hpp"
#include "episode_one_test_support.hpp"
#include "items/player_automatic_items.hpp"
#include "items/player_inventory.hpp"
#include "world/enemy_death_rewards.hpp"
#include "world/retail_save_file.hpp"
#include "world/retail_save_progress.hpp"
#include "world/scenario_data.hpp"
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

bool openAlexConversation(osf::WorldScene& world) {
    return osf::test::openNpcConversation(world, 0);
}

bool openRosannaConversation(
    osf::WorldScene& world,
    std::vector<std::int32_t>* audio = nullptr) {
    return osf::test::openNpcConversation(world, 4, audio);
}

osf::RetailSaveProgress coldSvaltProgress(
    const osf::WorldScene& seed,
    std::int32_t ruby_state) {
    osf::RetailSaveProgress progress =
        seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    progress.quest_flags[0] = 2;
    progress.quest_flags[1] = 2;
    progress.quest_flags[2] = 2;
    progress.quest_flags[3] = 2;
    progress.quest_flags[4] = ruby_state;
    if (progress.script_state_flags.size() < 72) {
        progress.script_state_flags.resize(72, 0);
    }
    progress.script_state_flags[2] = 1;
    progress.script_state_flags[4] = 1;
    progress.script_state_flags[6] = 2;
    progress.script_state_flags[7] = 1;
    progress.script_state_flags[8] = 1;
    if (ruby_state != 0) {
        progress.script_state_flags[11] = 1;
        progress.script_state_flags[15] = 1;
    }
    return progress;
}

bool writeFixture(
    const std::filesystem::path& save_path,
    const osf::WorldScene& world,
    const osf::PlayerData& player,
    const osf::RetailSaveProgress& progress,
    const osf::PlayerAutomaticItems& automatic_items,
    std::string& error,
    std::int32_t scenario = 1000000,
    std::int32_t entry = 0) {
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
        {true, scenario, entry},
        world.playerGiantWarehouse(),
        automatic_items,
        0x46,
        &error);
}

bool enterColdSvaltTown(osf::WorldScene& world) {
    const auto trigger = std::find_if(
        world.scenarioObjects().begin(),
        world.scenarioObjects().end(),
        [](const osf::ScenarioObjectActor& object) {
            return object.characterNumber() == 10000002;
        });
    if (trigger == world.scenarioObjects().end()) {
        return false;
    }
    const osf::ObjectBounds& bounds = trigger->judgement();
    const osf::WorldPosition target{
        trigger->position().x +
            (bounds.left + bounds.right) / 2,
        trigger->position().y +
            (bounds.top + bounds.bottom) / 2,
    };
    for (std::int32_t update = 0;
         update < 2000 && world.scenarioId() == 1000001;
         ++update) {
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
    return world.scenarioId() == 1000000 &&
           world.retailSaveWorldState().entry_value == 0;
}

bool testWildIceDrop(
    const std::filesystem::path& data_root,
    const osf::WorldScene& seed) {
    osf::ScenarioData occupied_town;
    std::string error;
    if (!check(
            occupied_town.load(
                data_root / "Scenario" / "01000001" /
                    "Scenario.Mct",
                &error),
            "The occupied Cold Svalt outskirts could not be decoded.")) {
        std::cerr << error << '\n';
        return false;
    }
    const auto wild_ice = std::find_if(
        occupied_town.enemies().begin(),
        occupied_town.enemies().end(),
        [](const osf::ScenarioEnemy& enemy) {
            return enemy.id == 1 && enemy.name == "Wild Ice";
        });
    if (!check(
            wild_ice != occupied_town.enemies().end() &&
                wild_ice->loot_table_row == 56,
            "Wild Ice no longer owns the authored Memorable Ruby row.")) {
        return false;
    }

    osf::RetailRandom random(1);
    const std::vector<osf::EnemyDeathDrop> drops =
        osf::createRetailEnemyDrops(
            wild_ice->loot_table_row,
            wild_ice->gold_drop_chance,
            wild_ice->gold_minimum,
            wild_ice->gold_maximum,
            {1000, 2000},
            {
                wild_ice->judgement_left,
                wild_ice->judgement_top,
                wild_ice->judgement_right,
                wild_ice->judgement_bottom,
            },
            0,
            1,
            1,
            seed.parameterTables(),
            seed.itemDatabase(),
            random);
    const osf::ItemDefinition* ruby =
        seed.itemDatabase().find(4, 99000002);
    const auto ruby_drop = std::find_if(
        drops.begin(),
        drops.end(),
        [](const osf::EnemyDeathDrop& drop) {
            return drop.item.category == 4 &&
                   drop.item.definition_id == 99000002;
        });
    return check(
        ruby && ruby->automatic_inventory_page == 0 &&
            ruby->automatic_inventory_x == 2 &&
            ruby->automatic_inventory_y == 0 &&
            ruby_drop != drops.end(),
        "Wild Ice did not produce the fixed automatic-owner Memorable Ruby.");
}

bool testMemorableRubyQuest(
    const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "MemorableRuby";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The Memorable Ruby fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    if (!testWildIceDrop(data_root, seed)) {
        return false;
    }
    osf::PlayerData level_thirty = seed.playerData();
    if (!check(
            raiseToLevel(
                level_thirty, 30, seed.parameterTables()),
            "The Memorable Ruby fixture could not reach Cold Svalt level.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_one_memorable_ruby_test";
    const std::filesystem::path offer_save =
        fixture_root / "offer" / "Save" / "0000.Ssv";
    const std::filesystem::path return_save =
        fixture_root / "return" / "Save" / "0000.Ssv";
    const std::filesystem::path completed_save =
        fixture_root / "completed" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);

    osf::PlayerAutomaticItems no_automatic_items;
    if (!check(
            writeFixture(
                offer_save,
                seed,
                level_thirty,
                coldSvaltProgress(seed, 0),
                no_automatic_items,
                error,
                1000001,
                2),
            "The first Cold Svalt visit could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene offered;
    const bool offer_loaded = loadSavedFixture(
        data_root, offer_save, offered, error);
    const bool town_entered =
        offer_loaded && offered.scenarioId() == 1000001 &&
        offered.enemies().size() == 108 &&
        enterColdSvaltTown(offered);
    const bool alex_opened =
        town_entered && openAlexConversation(offered);
    if (!check(
            town_entered &&
                offered.scenarioId() == 1000000 &&
                offered.scenario().title() == "Cold Svalt Town" &&
                offered.npcs().size() == 12 &&
                alex_opened &&
                offered.conversationMessageId() == 1000000,
            "Alex did not begin the authored Cold Svalt introduction.")) {
        std::cerr << error << " loaded=" << offer_loaded
                  << " entered=" << town_entered
                  << " scenario=" << offered.scenarioId()
                  << " title=" << offered.scenario().title()
                  << " npcs=" << offered.npcs().size()
                  << " opened=" << alex_opened
                  << " message=" << offered.conversationMessageId()
                  << '\n';
        return false;
    }
    for (const std::int32_t message : {
             1000001, 1000002, 1000003,
             1000004, 1000005, 1000006}) {
        offered.advanceConversation();
        if (!check(
                offered.conversationMessageId() == message,
                "Alex's Cold Svalt introduction skipped a message.")) {
            return false;
        }
    }
    offered.advanceConversation();
    if (!check(
            !offered.conversationActive() &&
                offered.retailSaveProgress()
                        .script_state_flags[11] == 1,
            "Alex did not release and latch his first introduction.")) {
        return false;
    }

    if (!check(
            openRosannaConversation(offered) &&
                offered.conversationMessageId() == 1000047,
            "Rosanna did not begin her authored introduction.")) {
        return false;
    }
    for (const std::int32_t message : {1000048, 1000049}) {
        offered.advanceConversation();
        if (!check(
                offered.conversationMessageId() == message,
                "Rosanna's introduction skipped a message.")) {
            return false;
        }
    }
    offered.advanceConversation();
    std::vector<std::int32_t> offer_audio;
    if (!check(
            !offered.conversationActive() &&
                openRosannaConversation(offered, &offer_audio) &&
                offered.conversationMessageId() == 1000050,
            "Rosanna did not continue with the missing-ruby request.")) {
        return false;
    }
    offered.advanceConversation();
    const std::vector<std::int32_t> quest_audio =
        offered.takeAudioSamples();
    if (!check(
            offered.conversationMessageId() == 1000051 &&
                offered.quests().state(4) == 1 &&
                offered.quests().lastCue() ==
                    osf::QuestCue::updated &&
                offered.quests().notice().quest_id == 4 &&
                offered.quests().notice().counter == 600 &&
                containsSample(quest_audio, 65),
            "Rosanna did not start mission four with its notice and cue.")) {
        return false;
    }
    offered.advanceConversation();
    if (!check(
            !offered.conversationActive(),
            "The Memorable Ruby offer did not release Rosanna.")) {
        return false;
    }

    const osf::ItemDefinition* ruby =
        offered.itemDatabase().find(4, 99000002);
    osf::PlayerAutomaticItems returned_items;
    if (!check(
            ruby && returned_items.add(
                        *ruby, osf::makeInventoryItem(*ruby)),
            "The Memorable Ruby could not enter its authored automatic page.")) {
        return false;
    }
    if (!check(
            writeFixture(
                return_save,
                offered,
                offered.playerData(),
                offered.retailSaveProgress(),
                returned_items,
                error),
            "The returned Memorable Ruby fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene returned;
    std::vector<std::int32_t> completion_audio;
    if (!check(
            loadSavedFixture(
                data_root, return_save, returned, error) &&
                returned.playerAutomaticItems().contains(
                    4, 99000002) &&
                openRosannaConversation(
                    returned, &completion_audio) &&
                returned.conversationMessageId() == 1000053 &&
                returned.quests().state(4) == 2 &&
                returned.quests().lastCue() ==
                    osf::QuestCue::completed &&
                !returned.playerAutomaticItems().contains(
                    4, 99000002) &&
                containsSample(completion_audio, 66),
            "Rosanna did not remove the ruby and complete mission four.")) {
        std::cerr << error << '\n';
        return false;
    }
    if (!check(
            returned.groundItems().empty(),
            "Rosanna created her reward before its authored callback.")) {
        return false;
    }
    returned.advanceConversation();
    if (!check(
            returned.conversationMessageId() == 1000054 &&
                returned.groundItems().size() == 1 &&
                returned.groundItems().front().item.category == 2 &&
                returned.groundItems().front().item.definition_id ==
                    1100003,
            "Rosanna's completion callback did not create its reward.")) {
        return false;
    }
    returned.takeAudioSamples();
    for (std::int32_t update = 0; update < 19; ++update) {
        returned.update();
    }
    const std::vector<std::int32_t> reward_audio =
        returned.takeAudioSamples();
    if (!check(
            std::count(
                reward_audio.begin(), reward_audio.end(), 93) == 1,
            "Rosanna's reward did not play its retail landing sound.")) {
        return false;
    }
    returned.advanceConversation();
    if (!check(
            !returned.conversationActive(),
            "Rosanna did not release the completed mission conversation.")) {
        return false;
    }

    if (!check(
            writeFixture(
                completed_save,
                returned,
                returned.playerData(),
                returned.retailSaveProgress(),
                returned.playerAutomaticItems(),
                error),
            "The completed Memorable Ruby mission could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene persisted;
    const bool completed =
        loadSavedFixture(
            data_root, completed_save, persisted, error) &&
        persisted.quests().state(4) == 2 &&
        !persisted.playerAutomaticItems().contains(4, 99000002) &&
        openRosannaConversation(persisted) &&
        persisted.conversationMessageId() == 1000055 &&
        persisted.groundItems().empty() &&
        !containsSample(persisted.takeAudioSamples(), 66);
    if (persisted.conversationActive()) {
        persisted.advanceConversation();
    }
    std::filesystem::remove_all(fixture_root, cleanup_error);
    return check(
        completed,
        "Saving and loading repeated Rosanna's mission or restored the ruby.");
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "01000001")) {
        return 0;
    }
    return testMemorableRubyQuest(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
