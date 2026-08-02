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

osf::RetailSaveProgress wizardProgress(
    const osf::WorldScene& seed) {
    osf::RetailSaveProgress progress = seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    for (const std::int32_t completed :
         {0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11}) {
        progress.quest_flags[
            static_cast<std::size_t>(completed)] = 2;
    }
    progress.quest_flags[12] = 1;
    progress.quest_flags[13] = 1;
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
        0x4f,
        &error);
}

bool testWizardTower(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "WizardTower";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The Wizard Tower fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerData level_sixty = seed.playerData();
    if (!check(
            raiseToLevel(level_sixty, 60, seed.parameterTables()),
            "The Wizard Tower fixture could not reach its area level.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_two_wizard_tower_test";
    const std::filesystem::path route_save =
        fixture_root / "route" / "Save" / "0000.Ssv";
    const std::filesystem::path persisted_save =
        fixture_root / "persisted" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);

    if (!check(
            writeFixture(
                route_save,
                seed,
                level_sixty,
                wizardProgress(seed),
                {true, 2100004, 0},
                error),
            "The active Kirushutat mission fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene route;
    if (!check(
            loadSavedFixture(data_root, route_save, route, error) &&
                route.quests().state(13) == 1 &&
                route.transitionScenario(
                    {2100004, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 1, 2100005) &&
                route.scenario().title() == "Forest of Sprits" &&
                route.retailSaveWorldState().entry_value == 0 &&
                route.transitionScenario(
                    {2100005, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 1, 2110000) &&
                route.scenario().title() ==
                    "Tower of the Wizard" &&
                route.retailSaveWorldState().entry_value == 0,
            "The authored Kirushutat route did not reach the Wizard Tower.")) {
        std::cerr << "scenario=" << route.scenarioId()
                  << " title=" << route.scenario().title()
                  << " entry="
                  << route.retailSaveWorldState().entry_value
                  << '\n';
        return false;
    }

    if (!check(
            route.transitionScenario(
                {2110000, 18, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                route.retailSaveWorldState().entry_value == 18 &&
                osf::test::openNpcConversation(route, 0) &&
                route.conversationMessageId() == 1000012 &&
                route.quests().state(13) == 2,
            "Kirushutat did not recognize and complete mission thirteen.")) {
        std::cerr << "message=" << route.conversationMessageId()
                  << " quest=" << route.quests().state(13)
                  << " player=" << route.playerWorldX() << ','
                  << route.playerWorldY() << '\n';
        return false;
    }

    for (const std::int32_t message :
         {1000013, 1000014, 1000015, 1000016, 1000017,
          1000018, 1000019, 1000020, 1000021, 1000022,
          1000023, 1000024, 1000025, 1000026, 1000027}) {
        route.advanceConversation();
        if (!check(
                route.conversationMessageId() == message,
                "Kirushutat's Seal Crystal briefing skipped a message.")) {
            std::cerr << "message="
                      << route.conversationMessageId()
                      << " expected=" << message << '\n';
            return false;
        }
    }
    const std::vector<std::int32_t> mission_audio =
        route.takeAudioSamples();
    const osf::MissionDefinition* mission =
        route.missions().find(14);
    if (!check(
            mission &&
                mission->title == "Take back the Seal Crystal." &&
                route.quests().state(13) == 2 &&
                route.quests().state(14) == 1 &&
                route.quests().notice().quest_id == 14 &&
                route.quests().notice().counter == 600 &&
                containsSample(mission_audio, 65),
            "Kirushutat did not start the Seal Crystal mission.")) {
        return false;
    }
    route.advanceConversation();
    if (!check(
            !route.conversationActive(),
            "Kirushutat did not release the Seal Crystal briefing.")) {
        return false;
    }

    if (!check(
            writeFixture(
                persisted_save,
                route,
                route.playerData(),
                route.retailSaveProgress(),
                {true, 2110000, 18},
                error),
            "The Seal Crystal mission state could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene persisted;
    const bool no_repeat =
        loadSavedFixture(
            data_root, persisted_save, persisted, error) &&
        persisted.scenarioId() == 2110000 &&
        persisted.retailSaveWorldState().entry_value == 18 &&
        persisted.quests().state(13) == 2 &&
        persisted.quests().state(14) == 1 &&
        osf::test::openNpcConversation(persisted, 0) &&
        persisted.conversationMessageId() == 1000028 &&
        persisted.groundItems().empty();
    if (persisted.conversationActive()) {
        persisted.advanceConversation();
    }
    std::filesystem::remove_all(fixture_root, cleanup_error);
    return check(
        no_repeat,
        "Saving the Seal Crystal handoff repeated Kirushutat's briefing.");
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "02110000")) {
        return 0;
    }
    return testWizardTower(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
