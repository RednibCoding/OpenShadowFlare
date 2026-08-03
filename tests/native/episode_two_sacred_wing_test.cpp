#include "core/retail_random.hpp"
#include "episode_one_test_support.hpp"
#include "items/item_instance_factory.hpp"
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

osf::RetailSaveProgress sacredWingProgress(
    const osf::WorldScene& seed) {
    osf::RetailSaveProgress progress = seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    for (const std::int32_t completed : {
             0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13, 14,
             15, 16, 17, 20}) {
        progress.quest_flags[
            static_cast<std::size_t>(completed)] = 2;
    }
    if (progress.script_state_flags.size() < 105) {
        progress.script_state_flags.resize(105, 0);
    }
    progress.script_state_flags[11] = 2;
    progress.script_state_flags[15] = 1;
    progress.script_state_flags[23] = 1;
    progress.script_state_flags[24] = 1;
    progress.script_state_flags[38] = 1;
    progress.script_state_flags[39] = 2;
    progress.script_state_flags[40] = 1;
    progress.script_state_flags[41] = 4;
    progress.script_state_flags[45] = 1;
    progress.script_state_flags[71] = 1;
    progress.script_state_flags[74] = 2;
    progress.script_state_flags[104] = 1;
    if (progress.transport_flags.size() < 51) {
        progress.transport_flags.resize(51, 0);
    }
    progress.transport_flags[25] = 1;
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
        0x5a,
        &error);
}

bool advanceMessages(
    osf::WorldScene& world,
    const std::vector<std::int32_t>& messages,
    const char* failure) {
    for (const std::int32_t message : messages) {
        world.advanceConversation();
        if (!check(world.conversationMessageId() == message, failure)) {
            std::cerr << "message=" << world.conversationMessageId()
                      << " expected=" << message << '\n';
            return false;
        }
    }
    return true;
}

const osf::ScenarioEnemy* sacredWingCarrier(
    const osf::ScenarioData& floor) {
    const auto found = std::find_if(
        floor.enemies().begin(),
        floor.enemies().end(),
        [](const osf::ScenarioEnemy& enemy) {
            return enemy.id == 0 &&
                   enemy.name == "Dark Golem" &&
                   enemy.loot_table_row == 154;
        });
    return found == floor.enemies().end() ? nullptr : &*found;
}

bool testSacredWingDrop(
    const std::filesystem::path& data_root,
    const osf::WorldScene& seed) {
    osf::ScenarioData fifth_floor;
    std::string error;
    if (!check(
            fifth_floor.load(
                data_root / "Scenario" / "03020004" /
                    "Scenario.Mct",
                &error),
            "Tower of Nazzle 5F could not be decoded.")) {
        std::cerr << error << '\n';
        return false;
    }
    const osf::ScenarioEnemy* carrier =
        sacredWingCarrier(fifth_floor);
    if (!check(
            carrier && fifth_floor.enemies().size() == 22,
            "Nazzle 5F lost its authored Sacred Wing carrier.")) {
        return false;
    }

    osf::RetailRandom random(1);
    const std::vector<osf::EnemyDeathDrop> drops =
        osf::createRetailEnemyDrops(
            carrier->loot_table_row,
            carrier->gold_drop_chance,
            carrier->gold_minimum,
            carrier->gold_maximum,
            {carrier->world_x, carrier->world_y},
            {
                carrier->judgement_left,
                carrier->judgement_top,
                carrier->judgement_right,
                carrier->judgement_bottom,
            },
            0,
            1,
            1,
            seed.parameterTables(),
            seed.itemDatabase(),
            random);
    const osf::ItemDefinition* wing =
        seed.itemDatabase().find(4, 99000005);
    const auto fixed_drop = std::find_if(
        drops.begin(),
        drops.end(),
        [](const osf::EnemyDeathDrop& drop) {
            return drop.item.category == 4 &&
                   drop.item.definition_id == 99000005;
        });
    return check(
        wing && wing->name == "Sacred Wing" &&
            wing->automatic_inventory_page == 0 &&
            wing->automatic_inventory_x == 5 &&
            wing->automatic_inventory_y == 0 &&
            fixed_drop != drops.end(),
        "The Dark Golem did not produce its fixed Sacred Wing item.");
}

bool clearNazzleFloor(
    osf::WorldScene& world,
    std::int32_t expected_scenario,
    std::int32_t expected_enemy_count,
    std::int32_t next_scenario) {
    if (!check(
            world.scenarioId() == expected_scenario &&
                static_cast<std::int32_t>(world.enemies().size()) ==
                    expected_enemy_count &&
                osf::test::scriptedObjectVisible(
                    world, 10011000, true),
            "A Nazzle combat floor did not begin behind its authored gate.")) {
        std::cerr << "scenario=" << world.scenarioId()
                  << " enemies=" << world.enemies().size() << '\n';
        return false;
    }
    if (!check(
            osf::test::markScenarioEnemiesDefeated(
                world, 0, expected_enemy_count - 1),
            "A Nazzle combat floor lost part of its authored roster.")) {
        return false;
    }
    for (std::int32_t update = 0;
         update < 500 &&
         osf::test::scriptedObjectVisible(
             world, 10011000, true);
         ++update) {
        world.update();
        world.takeAudioSamples();
    }
    if (!check(
            osf::test::scriptedObjectVisible(
                world, 10011000, false) &&
                osf::test::walkThroughScenarioTrigger(
                    world, 1, next_scenario),
            "Clearing a Nazzle floor did not open its upper stair.")) {
        return false;
    }
    return true;
}

bool testSacredWingMission(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "SacredWing";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The Sacred Wing fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    if (!testSacredWingDrop(data_root, seed)) {
        return false;
    }
    osf::PlayerData level_sixty = seed.playerData();
    if (!check(
            raiseToLevel(level_sixty, 60, seed.parameterTables()),
            "The Sacred Wing fixture could not reach its area level.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_two_sacred_wing_test";
    const std::filesystem::path arrival_save =
        fixture_root / "arrival" / "Save" / "0000.Ssv";
    const std::filesystem::path wing_save =
        fixture_root / "wing" / "Save" / "0000.Ssv";
    const std::filesystem::path completed_save =
        fixture_root / "completed" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);

    if (!check(
            writeFixture(
                arrival_save,
                seed,
                level_sixty,
                sacredWingProgress(seed),
                {true, 3900000, 50},
                seed.playerAutomaticItems(),
                error),
            "The Morris mission fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene route;
    if (!check(
            loadSavedFixture(data_root, arrival_save, route, error) &&
                osf::test::openNpcConversation(route, 3) &&
                route.conversationMessageId() == 1000027 &&
                route.retailSaveProgress().script_state_flags[77] == 1,
            "Morris did not begin with his separate trouble remark.")) {
        std::cerr << error << " message="
                  << route.conversationMessageId() << '\n';
        return false;
    }
    route.advanceConversation();
    if (!check(
            !route.conversationActive() &&
                osf::test::openNpcConversation(route, 3) &&
                route.conversationMessageId() == 1000028 &&
                route.retailSaveProgress().script_state_flags[77] == 2,
            "Morris did not recognize the dragon slayer on the next visit.")) {
        return false;
    }
    if (!advanceMessages(
            route,
            {1000029, 1000030, 1000031, 1000032, 1000033},
            "Morris's Sacred Wing briefing skipped a message.")) {
        return false;
    }
    const std::vector<std::int32_t> offer_audio =
        route.takeAudioSamples();
    const osf::MissionDefinition* mission = route.missions().find(21);
    if (!check(
            mission && mission->title ==
                           "Get the sacred relic, Sacred Wing." &&
                route.quests().state(21) == 1 &&
                route.quests().notice().quest_id == 21 &&
                containsSample(offer_audio, 65),
            "Morris did not start mission twenty-one with its retail cue.")) {
        return false;
    }
    route.advanceConversation();

    const osf::ScenarioTravelResult east_relocation =
        route.transitionScenario({3900000, 0, 0}, &error);
    const bool reached_east =
        east_relocation == osf::ScenarioTravelResult::relocated &&
        osf::test::walkThroughScenarioTrigger(route, 0, 3000507);
    const osf::ScenarioTravelResult tower_relocation =
        reached_east
            ? route.transitionScenario({3000507, 3, 0}, &error)
            : osf::ScenarioTravelResult::failed;
    const bool reached_tower =
        tower_relocation == osf::ScenarioTravelResult::relocated &&
        osf::test::walkThroughScenarioTrigger(route, 3, 3020000) &&
        route.transitionScenario({3020000, 200, 0}, &error) ==
            osf::ScenarioTravelResult::relocated;
    if (!check(
            reached_tower &&
                route.scenario().title() == "Tower of Nazzle, 1F" &&
                osf::test::openNpcConversation(route, 0) &&
                route.conversationMessageId() == 1000007 &&
                route.retailSaveProgress().script_state_flags[79] == 1,
            "Nazzle's first refusal did not record Edgar's report.")) {
        std::cerr << error << " scenario=" << route.scenarioId()
                  << " message=" << route.conversationMessageId()
                  << " east=" << reached_east
                  << " relocate="
                  << static_cast<std::int32_t>(tower_relocation)
                  << " tower=" << reached_tower << '\n';
        return false;
    }
    route.advanceConversation();

    if (!check(
            route.transitionScenario({3020000, 0, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 0, 3000507) &&
                route.transitionScenario({3000507, 0, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 0, 3900000) &&
                osf::test::openNpcConversation(route, 3) &&
                route.conversationMessageId() == 1000035 &&
                route.retailSaveProgress().script_state_flags[77] == 3,
            "Morris did not react to Nazzle and authorize Antalusia.")) {
        std::cerr << error << " scenario=" << route.scenarioId()
                  << " message=" << route.conversationMessageId() << '\n';
        return false;
    }
    if (!advanceMessages(
            route,
            {1000036, 1000037},
            "Morris's Berini letter handoff skipped a message.")) {
        return false;
    }
    route.advanceConversation();

    if (!check(
            route.transitionScenario({3900000, 0, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 0, 3000507) &&
                route.transitionScenario({3000507, 2, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 2, 3900001) &&
                route.transitionScenario({3900001, 2, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                route.scenario().title() == "A Town of Antalusia" &&
                osf::test::openNpcConversation(route, 0) &&
                route.conversationMessageId() == 1000002 &&
                route.retailSaveProgress().script_state_flags[80] == 1,
            "Berini did not accept Morris's introduction letter.")) {
        std::cerr << error << " scenario=" << route.scenarioId()
                  << " message=" << route.conversationMessageId() << '\n';
        return false;
    }
    if (!advanceMessages(
            route,
            {1000003, 1000004, 1000005, 1000006, 1000007,
             1000008},
            "Berini's introduction-letter response skipped a message.")) {
        return false;
    }
    route.advanceConversation();

    if (!check(
            route.transitionScenario({3900001, 0, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 0, 3000507) &&
                route.transitionScenario({3000507, 3, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 3, 3020000) &&
                route.transitionScenario({3020000, 200, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::openNpcConversation(route, 0) &&
                route.conversationMessageId() == 1000002 &&
                route.retailSaveProgress().script_state_flags[79] == 2,
            "Berini's authority did not make Edgar open Nazzle.")) {
        std::cerr << error << " scenario=" << route.scenarioId()
                  << " message=" << route.conversationMessageId() << '\n';
        return false;
    }
    if (!advanceMessages(
            route,
            {1000003, 1000004},
            "Edgar's tower warning skipped a message.")) {
        return false;
    }
    route.advanceConversation();
    route.update();
    if (!check(
            osf::test::scriptedObjectVisible(
                route, 10011000, false) &&
                route.transitionScenario({3020000, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 1, 3020001),
            "Nazzle's authorized first-floor stair did not open.")) {
        return false;
    }

    route.update();
    if (!clearNazzleFloor(route, 3020001, 25, 3020002)) {
        return false;
    }
    route.update();
    if (!clearNazzleFloor(route, 3020002, 25, 3020003)) {
        return false;
    }
    route.update();
    if (!clearNazzleFloor(route, 3020003, 29, 3020004)) {
        return false;
    }
    if (!check(
            route.scenario().title() == "Tower of Nazzle, 5F" &&
                route.enemies().size() == 22,
            "The authored Nazzle climb did not reach its fifth floor.")) {
        return false;
    }

    const osf::ItemDefinition* wing =
        route.itemDatabase().find(4, 99000005);
    osf::PlayerAutomaticItems recovered_items =
        route.playerAutomaticItems();
    if (!check(
            wing && recovered_items.add(
                        *wing, osf::makeInventoryItem(*wing)) &&
                writeFixture(
                    wing_save,
                    route,
                    route.playerData(),
                    route.retailSaveProgress(),
                    {true, 3020004, 0},
                    recovered_items,
                    error),
            "The recovered Sacred Wing could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene returned;
    if (!check(
            loadSavedFixture(data_root, wing_save, returned, error) &&
                returned.playerAutomaticItems().contains(
                    4, 99000005) &&
                osf::test::walkThroughScenarioTrigger(
                    returned, 0, 3020003) &&
                returned.transitionScenario({3020003, 0, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    returned, 0, 3020002) &&
                returned.transitionScenario({3020002, 0, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    returned, 0, 3020001) &&
                returned.transitionScenario({3020001, 0, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    returned, 0, 3020000) &&
                returned.transitionScenario({3020000, 0, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    returned, 0, 3000507) &&
                returned.transitionScenario({3000507, 2, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    returned, 2, 3900001) &&
                returned.transitionScenario({3900001, 2, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated,
            "The Sacred Wing return route did not reach Berini.")) {
        std::cerr << error << " scenario=" << returned.scenarioId()
                  << '\n';
        return false;
    }

    const std::int32_t experience_before =
        returned.playerData().experience();
    const std::int32_t expected_experience =
        experience_before +
        returned.playerData().experienceThreshold(
            returned.parameterTables()) * 50 / 100;
    std::vector<std::int32_t> reward_audio;
    if (!check(
            osf::test::openNpcConversation(
                returned, 0, &reward_audio) &&
                returned.conversationMessageId() == 1000010 &&
                returned.quests().state(21) == 2 &&
                returned.retailSaveProgress().script_state_flags[80] == 2 &&
                !returned.playerAutomaticItems().contains(
                    4, 99000005) &&
                returned.playerData().experience() ==
                    expected_experience,
            "Berini did not accept and reward the Sacred Wing exactly once.")) {
        std::cerr << "message=" << returned.conversationMessageId()
                  << " quest=" << returned.quests().state(21) << '\n';
        return false;
    }
    const std::vector<std::int32_t> queued_reward_audio =
        returned.takeAudioSamples();
    reward_audio.insert(
        reward_audio.end(),
        queued_reward_audio.begin(),
        queued_reward_audio.end());
    if (!check(
            containsSample(reward_audio, 64) &&
                containsSample(reward_audio, 66),
            "The Sacred Wing return lost its reward or completion cue.")) {
        return false;
    }
    if (!advanceMessages(
            returned,
            {1000011, 1000012, 1000013, 1000014, 1000015, 1000016},
            "Berini's Sacred Wing reward skipped a message.")) {
        return false;
    }
    returned.advanceConversation();
    const std::int32_t rewarded_experience =
        returned.playerData().experience();
    if (!check(
            returned.playerGiantWarehouse().pageEnabled(2),
            "Berini's reward did not unlock Giant Warehouse III.")) {
        return false;
    }

    if (!check(
            writeFixture(
                completed_save,
                returned,
                returned.playerData(),
                returned.retailSaveProgress(),
                {true, 3900001, 2},
                returned.playerAutomaticItems(),
                error),
            "The completed Sacred Wing mission could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene persisted;
    std::vector<std::int32_t> repeat_audio;
    const bool no_repeat =
        loadSavedFixture(
            data_root, completed_save, persisted, error) &&
        persisted.quests().state(21) == 2 &&
        persisted.retailSaveProgress().script_state_flags[77] == 3 &&
        persisted.retailSaveProgress().script_state_flags[79] == 2 &&
        persisted.retailSaveProgress().script_state_flags[80] == 2 &&
        !persisted.playerAutomaticItems().contains(4, 99000005) &&
        persisted.playerGiantWarehouse().pageEnabled(2) &&
        persisted.playerData().experience() == rewarded_experience &&
        osf::test::openNpcConversation(
            persisted, 0, &repeat_audio) &&
        persisted.conversationMessageId() == 1000017 &&
        persisted.playerData().experience() == rewarded_experience &&
        !containsSample(repeat_audio, 64) &&
        !containsSample(persisted.takeAudioSamples(), 64);
    std::filesystem::remove_all(fixture_root, cleanup_error);
    return check(
        no_repeat,
        "Saving the Sacred Wing return repeated its item or XP reward.");
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "03900000") ||
        !std::filesystem::is_directory(
            data_root / "Scenario" / "03020004")) {
        return 0;
    }
    return testSacredWingMission(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
