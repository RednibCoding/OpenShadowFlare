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

osf::RetailSaveProgress routeProgress(
    const osf::WorldScene& seed,
    std::int32_t scouting_state,
    std::int32_t battle_state = 0) {
    osf::RetailSaveProgress progress = seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    for (const std::int32_t completed :
         {0, 1, 2, 3, 4, 6, 7, 8}) {
        progress.quest_flags[static_cast<std::size_t>(completed)] = 2;
    }
    progress.quest_flags[9] = scouting_state;
    progress.quest_flags[10] = battle_state;
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
        0x4a,
        &error);
}

bool testImmortalRoute(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "ImmortalRoute";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The Immortal route fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerData level_sixty = seed.playerData();
    if (!check(
            raiseToLevel(level_sixty, 60, seed.parameterTables()),
            "The Immortal route fixture could not reach its area level.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_one_immortal_route_test";
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
                routeProgress(seed, 1),
                {true, 1040002, 5},
                error),
            "The active scouting route fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene route;
    std::vector<std::int32_t> transition_audio;
    if (!check(
            loadSavedFixture(data_root, route_save, route, error) &&
                route.scenarioId() == 1040002 &&
                osf::test::walkThroughScenarioTrigger(
                    route, 5, 1000004) &&
                route.scenario().title() == "Sea of Trees" &&
                route.retailSaveWorldState().entry_value == 1 &&
                route.transitionScenario(
                    {1000004, 0, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route,
                    0,
                    1050000,
                    5000,
                    &transition_audio) &&
                route.scenario().title() == "Immortal Remains" &&
                route.retailSaveWorldState().entry_value == 0 &&
                route.quests().state(9) == 2 &&
                route.quests().lastCue() ==
                    osf::QuestCue::completed &&
                containsSample(transition_audio, 66),
            "Entering Immortal Remains did not complete the scouting mission.")) {
        std::cerr << error << '\n';
        return false;
    }

    if (!check(
            writeFixture(
                return_save,
                route,
                route.playerData(),
                route.retailSaveProgress(),
                {true, 1000000, 0},
                error),
            "The completed scouting fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene returned;
    if (!check(
            loadSavedFixture(data_root, return_save, returned, error) &&
                osf::test::openNpcConversation(returned, 0) &&
                returned.conversationMessageId() == 1000026 &&
                returned.groundItems().empty(),
            "Alex's Immortal Remains report branch differs from retail.")) {
        return false;
    }
    returned.advanceConversation();
    if (!check(
            returned.conversationMessageId() == 1000027,
            "Alex skipped the Immortal Remains mission introduction.")) {
        return false;
    }
    returned.advanceConversation();
    const std::vector<std::int32_t> offer_audio =
        returned.takeAudioSamples();
    if (!check(
            returned.conversationMessageId() == 1000028 &&
                returned.quests().state(10) == 1 &&
                returned.quests().notice().quest_id == 10 &&
                returned.quests().notice().counter == 600 &&
                containsSample(offer_audio, 65) &&
                returned.groundItems().empty(),
            "Alex did not start the authored Immortal Remains battle.")) {
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
            "The Immortal Remains assignment could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene persisted;
    const bool completed =
        loadSavedFixture(
            data_root, completed_save, persisted, error) &&
        persisted.quests().state(9) == 2 &&
        persisted.quests().state(10) == 1 &&
        osf::test::openNpcConversation(persisted, 0) &&
        persisted.conversationMessageId() == 1000029 &&
        persisted.groundItems().empty();
    if (persisted.conversationActive()) {
        persisted.advanceConversation();
    }
    std::filesystem::remove_all(fixture_root, cleanup_error);
    return check(
        completed,
        "Saving and loading repeated the scouting handoff.");
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "01050000")) {
        return 0;
    }
    return testImmortalRoute(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
