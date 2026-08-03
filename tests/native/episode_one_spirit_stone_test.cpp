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

bool openSyriaConversation(
    osf::WorldScene& world,
    std::vector<std::int32_t>* audio = nullptr) {
    return osf::test::openNpcConversation(world, 2, audio);
}

osf::RetailSaveProgress spiritStoneProgress(
    const osf::WorldScene& seed,
    std::int32_t quest_state) {
    osf::RetailSaveProgress progress =
        seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    progress.quest_flags[0] = 2;
    progress.quest_flags[2] = quest_state;
    progress.quest_flags[3] = 1;
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
    const osf::PlayerAutomaticItems& automatic_items,
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
        {false, 0, 0},
        world.playerGiantWarehouse(),
        automatic_items,
        0x43,
        &error);
}

bool testStoneSpikeDrop(
    const std::filesystem::path& data_root,
    const osf::WorldScene& seed) {
    osf::ScenarioData continued_room;
    std::string error;
    if (!check(
            continued_room.load(
                data_root / "Scenario" / "00010005" /
                    "Scenario.Mct",
                &error),
            "The continued Dusty Ruins room could not be decoded.")) {
        std::cerr << error << '\n';
        return false;
    }
    const auto stone_spike = std::find_if(
        continued_room.enemies().begin(),
        continued_room.enemies().end(),
        [](const osf::ScenarioEnemy& enemy) {
            return enemy.id == 1 && enemy.name == "Stone Spike";
        });
    if (!check(
            stone_spike != continued_room.enemies().end() &&
                stone_spike->loot_table_row == 23,
            "Stone Spike no longer owns the authored Spirit Stone row.")) {
        return false;
    }

    osf::RetailRandom random(1);
    const std::vector<osf::EnemyDeathDrop> drops =
        osf::createRetailEnemyDrops(
            stone_spike->loot_table_row,
            stone_spike->gold_drop_chance,
            stone_spike->gold_minimum,
            stone_spike->gold_maximum,
            {1000, 2000},
            {
                stone_spike->judgement_left,
                stone_spike->judgement_top,
                stone_spike->judgement_right,
                stone_spike->judgement_bottom,
            },
            0,
            1,
            1,
            seed.parameterTables(),
            seed.itemDatabase(),
            random);
    const osf::ItemDefinition* spirit_stone =
        seed.itemDatabase().find(4, 99000001);
    const bool exact_drop =
        spirit_stone &&
            spirit_stone->automatic_inventory_page == 0 &&
            spirit_stone->automatic_inventory_x == 1 &&
            spirit_stone->automatic_inventory_y == 0 &&
            drops.size() == 1 &&
            drops.front().item.category == 4 &&
            drops.front().item.definition_id == 99000001 &&
            drops.front().position.x == 1200 &&
            drops.front().position.y == 2000;
    if (!check(
            exact_drop,
            "Stone Spike did not create the fixed automatic-owner Spirit Stone.")) {
        std::cerr << "definition=" << (spirit_stone != nullptr)
                  << " page="
                  << (spirit_stone
                          ? spirit_stone->automatic_inventory_page
                          : -1)
                  << " cell="
                  << (spirit_stone
                          ? spirit_stone->automatic_inventory_x
                          : -1)
                  << ','
                  << (spirit_stone
                          ? spirit_stone->automatic_inventory_y
                          : -1)
                  << " drops=" << drops.size() << '\n';
        for (const osf::EnemyDeathDrop& drop : drops) {
            std::cerr << "drop " << drop.item.category << '/'
                      << drop.item.definition_id << " at "
                      << drop.position.x << ',' << drop.position.y
                      << '\n';
        }
    }
    return exact_drop;
}

bool testSpiritStoneMission(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "SpiritStone";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The Spirit Stone fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    if (!testStoneSpikeDrop(data_root, seed)) {
        return false;
    }
    osf::PlayerData level_thirty = seed.playerData();
    if (!check(
            raiseToLevel(
                level_thirty, 30, seed.parameterTables()),
            "The Spirit Stone fixture could not match its mission-three gate.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_spirit_stone_quest_test";
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
                spiritStoneProgress(seed, 0),
                no_automatic_items,
                error),
            "The Spirit Stone offer fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene offered;
    std::vector<std::int32_t> offer_audio;
    if (!check(
            loadSavedFixture(data_root, offer_save, offered, error) &&
                openSyriaConversation(offered, &offer_audio) &&
                offered.conversationMessageId() == 1000044 &&
                offered.quests().state(2) == 1 &&
                offered.quests().lastCue() ==
                    osf::QuestCue::updated &&
                offered.quests().notice().quest_id == 2 &&
                offered.quests().notice().counter == 600 &&
                containsSample(offer_audio, 65),
            "Syria did not offer mission two with its retail notice and cue.")) {
        std::cerr << error << '\n';
        return false;
    }
    offered.advanceConversation();
    if (!check(
            !offered.conversationActive(),
            "The Spirit Stone offer did not release Syria.")) {
        return false;
    }

    const osf::ItemDefinition* spirit_stone =
        seed.itemDatabase().find(4, 99000001);
    osf::PlayerAutomaticItems returned_items;
    if (!check(
            spirit_stone && returned_items.add(
                *spirit_stone,
                osf::makeInventoryItem(*spirit_stone)),
            "The Spirit Stone could not enter its authored automatic page.")) {
        return false;
    }
    if (!check(
            writeFixture(
                return_save,
                seed,
                level_thirty,
                spiritStoneProgress(seed, 1),
                returned_items,
                error),
            "The Spirit Stone return fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene returned;
    std::vector<std::int32_t> completion_audio;
    if (!check(
            loadSavedFixture(data_root, return_save, returned, error) &&
                returned.playerAutomaticItems().contains(
                    4, 99000001) &&
                openSyriaConversation(
                    returned, &completion_audio) &&
                returned.conversationMessageId() == 1000045 &&
                returned.quests().state(2) == 2 &&
                returned.quests().lastCue() ==
                    osf::QuestCue::completed &&
                !returned.playerAutomaticItems().contains(
                    4, 99000001) &&
                containsSample(completion_audio, 66),
            "Syria did not remove the returned stone and complete mission two.")) {
        std::cerr << error << '\n';
        return false;
    }
    if (!check(
            returned.groundItems().empty(),
            "Syria created her reward before its authored callback.")) {
        return false;
    }
    returned.advanceConversation();
    if (!check(
            returned.conversationMessageId() == 1000046 &&
                returned.groundItems().size() == 1 &&
                returned.groundItems().front().item.category == 2 &&
                returned.groundItems().front().item.definition_id ==
                    1100001,
            "Syria's completion callback did not create its authored reward.")) {
        return false;
    }
    returned.advanceConversation();
    if (!check(
            !returned.conversationActive(),
            "The Spirit Stone completion did not release Syria.")) {
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
            "The completed Spirit Stone mission could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene persisted;
    const bool completed =
        loadSavedFixture(
            data_root, completed_save, persisted, error) &&
        persisted.quests().state(2) == 2 &&
        !persisted.playerAutomaticItems().contains(4, 99000001) &&
        openSyriaConversation(persisted) &&
        persisted.conversationMessageId() == 1000038 &&
        persisted.groundItems().empty() &&
        !containsSample(persisted.takeAudioSamples(), 66);
    if (persisted.conversationActive()) {
        persisted.advanceConversation();
    }
    std::filesystem::remove_all(fixture_root, cleanup_error);
    return check(
        completed,
        "Saving and loading repeated Syria's mission or restored the stone.");
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "00010005")) {
        return 0;
    }
    return testSpiritStoneMission(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
