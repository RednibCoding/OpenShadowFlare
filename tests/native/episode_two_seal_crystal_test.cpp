#include "core/retail_random.hpp"
#include "episode_one_test_support.hpp"
#include "items/player_automatic_items.hpp"
#include "world/enemy_death_rewards.hpp"
#include "world/retail_save_file.hpp"
#include "world/retail_save_progress.hpp"
#include "world/retail_save_world_state.hpp"
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

osf::RetailSaveProgress sealCrystalProgress(
    const osf::WorldScene& seed) {
    osf::RetailSaveProgress progress = seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    for (const std::int32_t completed :
         {0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 13}) {
        progress.quest_flags[
            static_cast<std::size_t>(completed)] = 2;
    }
    progress.quest_flags[12] = 1;
    progress.quest_flags[14] = 1;
    if (progress.script_state_flags.size() < 72) {
        progress.script_state_flags.resize(72, 0);
    }
    progress.script_state_flags[11] = 2;
    progress.script_state_flags[15] = 1;
    progress.script_state_flags[23] = 1;
    progress.script_state_flags[24] = 1;
    progress.script_state_flags[71] = 1;
    return progress;
}

bool writeFixture(
    const std::filesystem::path& save_path,
    const osf::WorldScene& world,
    const osf::PlayerData& player,
    const osf::RetailSaveProgress& progress,
    const osf::RetailSaveWorldState& world_state,
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
        world_state,
        world.playerGiantWarehouse(),
        automatic_items,
        0x50,
        &error);
}

const osf::ScenarioEnemy* findSealCrystalGuardian(
    const osf::ScenarioData& fort) {
    const auto guardian = std::find_if(
        fort.enemies().begin(),
        fort.enemies().end(),
        [](const osf::ScenarioEnemy& enemy) {
            return enemy.name == "Oak Warrior" &&
                   enemy.loot_table_row == 76;
        });
    return guardian == fort.enemies().end() ? nullptr : &*guardian;
}

bool testSealCrystalDrop(
    const std::filesystem::path& data_root,
    const osf::WorldScene& seed) {
    osf::ScenarioData fort;
    std::string error;
    if (!check(
            fort.load(
                data_root / "Scenario" / "02120000" /
                    "Scenario.Mct",
                &error),
            "The Fort of Thieves could not be decoded.")) {
        std::cerr << error << '\n';
        return false;
    }
    const osf::ScenarioEnemy* guardian =
        findSealCrystalGuardian(fort);
    if (!check(
            guardian && guardian->id == 65,
            "The Fort of Thieves no longer has its Seal Crystal guardian.")) {
        return false;
    }

    osf::RetailRandom random(1);
    const std::vector<osf::EnemyDeathDrop> drops =
        osf::createRetailEnemyDrops(
            guardian->loot_table_row,
            guardian->gold_drop_chance,
            guardian->gold_minimum,
            guardian->gold_maximum,
            {guardian->world_x, guardian->world_y},
            {
                guardian->judgement_left,
                guardian->judgement_top,
                guardian->judgement_right,
                guardian->judgement_bottom,
            },
            0,
            1,
            1,
            seed.parameterTables(),
            seed.itemDatabase(),
            random);
    const osf::ItemDefinition* crystal =
        seed.itemDatabase().find(4, 99000003);
    const auto crystal_drop = std::find_if(
        drops.begin(),
        drops.end(),
        [](const osf::EnemyDeathDrop& drop) {
            return drop.item.category == 4 &&
                   drop.item.definition_id == 99000003;
        });
    return check(
        crystal && crystal->name == "Seal Crystal" &&
            crystal->automatic_inventory_page == 0 &&
            crystal->automatic_inventory_x == 3 &&
            crystal->automatic_inventory_y == 0 &&
            crystal_drop != drops.end(),
        "The Oak Warrior did not produce the fixed Seal Crystal item.");
}

bool testSealCrystalMission(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "SealCrystal";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The Seal Crystal fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    if (!testSealCrystalDrop(data_root, seed)) {
        return false;
    }
    osf::PlayerData level_sixty = seed.playerData();
    if (!check(
            raiseToLevel(level_sixty, 60, seed.parameterTables()),
            "The Seal Crystal fixture could not reach its area level.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_two_seal_crystal_test";
    const std::filesystem::path route_save =
        fixture_root / "route" / "Save" / "0000.Ssv";
    const std::filesystem::path return_save =
        fixture_root / "return" / "Save" / "0000.Ssv";
    const std::filesystem::path completed_save =
        fixture_root / "completed" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);

    osf::PlayerAutomaticItems no_automatic_items;
    if (!check(
            writeFixture(
                route_save,
                seed,
                level_sixty,
                sealCrystalProgress(seed),
                {true, 2100004, 0},
                no_automatic_items,
                error),
            "The active Seal Crystal mission fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene route;
    if (!check(
            loadSavedFixture(data_root, route_save, route, error) &&
                route.quests().state(14) == 1 &&
                route.transitionScenario(
                    {2100004, 2, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 2, 2100006) &&
                route.scenario().title() ==
                    "Forest of Knight's Misery" &&
                route.retailSaveWorldState().entry_value == 0 &&
                route.transitionScenario(
                    {2100006, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 1, 2120000) &&
                route.scenario().title() == "Fort of Thieves" &&
                route.retailSaveWorldState().entry_value == 0,
            "The authored Seal Crystal route did not reach the thieves' fort.")) {
        std::cerr << "scenario=" << route.scenarioId()
                  << " title=" << route.scenario().title()
                  << " entry="
                  << route.retailSaveWorldState().entry_value
                  << '\n';
        return false;
    }
    const auto live_guardian = std::find_if(
        route.enemies().begin(),
        route.enemies().end(),
        [](const osf::EnemyActor& enemy) {
            return enemy.id() == 65 &&
                   enemy.name() == "Oak Warrior" &&
                   enemy.lootTableRow() == 76;
        });
    if (!check(
            live_guardian != route.enemies().end(),
            "The live Fort of Thieves lost its Seal Crystal guardian.")) {
        return false;
    }

    const osf::ItemDefinition* crystal =
        route.itemDatabase().find(4, 99000003);
    osf::PlayerAutomaticItems returned_items;
    if (!check(
            crystal && returned_items.add(
                           *crystal,
                           osf::makeInventoryItem(*crystal)),
            "The Seal Crystal could not enter its authored automatic page.")) {
        return false;
    }
    if (!check(
            writeFixture(
                return_save,
                route,
                route.playerData(),
                route.retailSaveProgress(),
                {true, 2110000, 18},
                returned_items,
                error),
            "The recovered Seal Crystal fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene returned;
    std::vector<std::int32_t> completion_audio;
    if (!check(
            loadSavedFixture(data_root, return_save, returned, error) &&
                returned.playerAutomaticItems().contains(
                    4, 99000003) &&
                osf::test::openNpcConversation(
                    returned, 0, &completion_audio) &&
                returned.conversationMessageId() == 1000029 &&
                returned.quests().state(12) == 1 &&
                returned.quests().state(13) == 2 &&
                returned.quests().state(14) == 2 &&
                !returned.playerAutomaticItems().contains(
                    4, 99000003) &&
                containsSample(completion_audio, 66),
            "Kirushutat did not remove the crystal and complete mission fourteen.")) {
        std::cerr << error << '\n';
        return false;
    }
    for (const std::int32_t message : {1000030, 1000031}) {
        returned.advanceConversation();
        if (!check(
                returned.conversationMessageId() == message,
                "Kirushutat's Seal Crystal return skipped a message.")) {
            std::cerr << "message="
                      << returned.conversationMessageId()
                      << " expected=" << message << '\n';
            return false;
        }
    }
    returned.advanceConversation();
    returned.advanceConversation();
    if (!check(
            !returned.conversationActive(),
            "Kirushutat did not release the Seal Crystal return.")) {
        return false;
    }

    if (!check(
            writeFixture(
                completed_save,
                returned,
                returned.playerData(),
                returned.retailSaveProgress(),
                {true, 2100004, 0},
                returned.playerAutomaticItems(),
                error),
            "The completed Seal Crystal mission could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene opened_gate;
    if (!check(
            loadSavedFixture(
                data_root, completed_save, opened_gate, error) &&
                opened_gate.quests().state(14) == 2 &&
                opened_gate.transitionScenario(
                    {2100004, 3, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    opened_gate, 3, 2200000) &&
                opened_gate.scenario().title() ==
                    "Fanann, Village of Elves" &&
                opened_gate.retailSaveWorldState().entry_value == 0,
            "Returning the Seal Crystal did not open the Fanann gate.")) {
        std::cerr << error << '\n';
        return false;
    }

    if (!check(
            writeFixture(
                completed_save,
                opened_gate,
                opened_gate.playerData(),
                opened_gate.retailSaveProgress(),
                {true, 2110000, 18},
                opened_gate.playerAutomaticItems(),
                error),
            "The post-crystal Kirushutat branch could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene persisted;
    const bool no_repeat =
        loadSavedFixture(
            data_root, completed_save, persisted, error) &&
        persisted.quests().state(14) == 2 &&
        !persisted.playerAutomaticItems().contains(4, 99000003) &&
        osf::test::openNpcConversation(persisted, 0) &&
        persisted.conversationMessageId() == 1000032 &&
        !containsSample(persisted.takeAudioSamples(), 66);
    if (persisted.conversationActive()) {
        persisted.advanceConversation();
    }
    std::filesystem::remove_all(fixture_root, cleanup_error);
    return check(
        no_repeat,
        "Saving the Seal Crystal return repeated its handoff.");
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "02120000")) {
        return 0;
    }
    return testSealCrystalMission(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
