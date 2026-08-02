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

namespace {

using osf::test::check;
using osf::test::loadSavedFixture;
using osf::test::raiseToLevel;

osf::RetailSaveProgress routeProgress(
    const osf::WorldScene& seed,
    std::int32_t dusty_ruins_state) {
    osf::RetailSaveProgress progress =
        seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    progress.quest_flags[0] = 2;
    progress.quest_flags[3] = dusty_ruins_state;
    if (progress.script_state_flags.size() < 72) {
        progress.script_state_flags.resize(72, 0);
    }
    progress.script_state_flags[2] =
        dusty_ruins_state == 2 ? 1 : 0;
    progress.script_state_flags[4] = 1;
    return progress;
}

bool writeRouteFixture(
    const std::filesystem::path& save_path,
    const osf::WorldScene& seed,
    const osf::PlayerData& player,
    std::int32_t dusty_ruins_state,
    std::int32_t scenario,
    std::int32_t entry,
    std::string& error) {
    return osf::writeRetailSave(
        save_path,
        player,
        seed.itemDatabase(),
        seed.playerInventory(),
        seed.playerEquipment(),
        seed.playerBelt(),
        seed.playerSpecialItems(),
        routeProgress(seed, dusty_ruins_state),
        seed.playerMagic(),
        seed.playerMineCount(),
        {true, scenario, entry},
        seed.playerGiantWarehouse(),
        seed.playerAutomaticItems(),
        0x45,
        &error);
}

const osf::ScenarioObjectActor* findTrigger(
    const osf::WorldScene& world,
    std::int32_t local_id) {
    const std::int32_t character_number = 10000000 + local_id;
    const auto found = std::find_if(
        world.scenarioObjects().begin(),
        world.scenarioObjects().end(),
        [character_number](
            const osf::ScenarioObjectActor& object) {
            return object.characterNumber() == character_number;
        });
    return found == world.scenarioObjects().end()
               ? nullptr
               : &*found;
}

osf::WorldPosition triggerCenter(
    const osf::ScenarioObjectActor& trigger) {
    const osf::ObjectBounds& bounds = trigger.judgement();
    return {
        trigger.position().x +
            (bounds.left + bounds.right) / 2,
        trigger.position().y +
            (bounds.top + bounds.bottom) / 2,
    };
}

bool walkThroughTrigger(
    osf::WorldScene& world,
    std::int32_t local_id,
    std::int32_t expected_scenario,
    std::int32_t maximum_updates = 5000) {
    const osf::ScenarioObjectActor* trigger =
        findTrigger(world, local_id);
    if (!trigger) {
        return false;
    }
    const std::int32_t source_scenario = world.scenarioId();
    const osf::WorldPosition target = triggerCenter(*trigger);
    for (std::int32_t update = 0;
         update < maximum_updates &&
         world.scenarioId() == source_scenario;
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
    if (world.scenarioId() != expected_scenario) {
        std::cerr << "route trigger " << local_id
                  << " from scenario " << source_scenario
                  << " ended in " << world.scenarioId()
                  << " at " << world.playerWorldX() << ','
                  << world.playerWorldY() << " targeting "
                  << target.x << ',' << target.y
                  << " life=" << world.playerData().currentLife()
                  << '\n';
        return false;
    }
    return true;
}

bool reachTriggerWithoutLeaving(
    osf::WorldScene& world,
    std::int32_t local_id,
    std::int32_t updates = 2000) {
    const osf::ScenarioObjectActor* trigger =
        findTrigger(world, local_id);
    if (!trigger) {
        return false;
    }
    const std::int32_t source_scenario = world.scenarioId();
    const osf::WorldPosition target = triggerCenter(*trigger);
    const osf::WorldPosition trigger_position = trigger->position();
    const osf::ObjectBounds bounds = trigger->judgement();
    for (std::int32_t update = 0;
         update < updates && world.scenarioId() == source_scenario;
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
    const osf::WorldPosition player{
        world.playerWorldX(), world.playerWorldY()};
    return world.scenarioId() == source_scenario &&
           player.x >= trigger_position.x + bounds.left &&
           player.x <= trigger_position.x + bounds.right &&
           player.y >= trigger_position.y + bounds.top &&
           player.y <= trigger_position.y + bounds.bottom;
}

bool testColdSvaltRoute(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "ColdSvaltRoute";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The Cold Svalt route fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerData level_thirty = seed.playerData();
    if (!check(
            raiseToLevel(
                level_thirty, 30, seed.parameterTables()),
            "The Cold Svalt route fixture could not reach level 30.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_one_cold_svalt_route_test";
    const std::filesystem::path complete_save =
        fixture_root / "complete" / "Save" / "0000.Ssv";
    const std::filesystem::path locked_save =
        fixture_root / "locked" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);

    if (!check(
            writeRouteFixture(
                complete_save,
                seed,
                level_thirty,
                2,
                1,
                0,
                error),
            "The completed Dusty Ruins route fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene route;
    if (!check(
            loadSavedFixture(
                data_root, complete_save, route, error) &&
                route.scenarioId() == 1 &&
                route.scenario().title() ==
                    "Near the Remote Town" &&
                route.quests().state(3) == 2,
            "The authored route did not begin near Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    if (!check(
            walkThroughTrigger(route, 6, 3) &&
                route.scenario().title() ==
                    "Wasteland of Hesitation" &&
                route.retailSaveWorldState().entry_value == 1,
            "The Near Remote Town edge did not enter Wasteland of Hesitation.")) {
        return false;
    }
    if (!check(
            route.transitionScenario({3, 0, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                walkThroughTrigger(route, 0, 5) &&
                route.scenario().title() == "Frozen Forest" &&
                route.retailSaveWorldState().entry_value == 0,
            "Wasteland of Hesitation did not lead into Frozen Forest.")) {
        std::cerr << error << '\n';
        return false;
    }
    if (!check(
            route.transitionScenario({5, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                walkThroughTrigger(route, 1, 6) &&
                route.scenario().title() ==
                    "Wasteland of Pillars" &&
                route.retailSaveWorldState().entry_value == 1,
            "Frozen Forest did not lead into Wasteland of Pillars.")) {
        std::cerr << error << '\n';
        return false;
    }
    if (!check(
            route.transitionScenario({6, 3, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                walkThroughTrigger(route, 3, 1000001) &&
                route.scenario().title() == "Cold Svalt Town" &&
                route.retailSaveWorldState().entry_value == 0 &&
                route.quests().state(3) == 2,
            "The completed Dusty Ruins gate did not enter occupied Cold Svalt.")) {
        return false;
    }

    if (!check(
            writeRouteFixture(
                locked_save,
                seed,
                level_thirty,
                1,
                6,
                3,
                error),
            "The locked Cold Svalt route fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene locked;
    const bool remains_locked =
        loadSavedFixture(data_root, locked_save, locked, error) &&
        locked.scenarioId() == 6 &&
        locked.quests().state(3) == 1 &&
        reachTriggerWithoutLeaving(locked, 3) &&
        locked.scenarioId() == 6;
    std::filesystem::remove_all(fixture_root, cleanup_error);
    return check(
        remains_locked,
        "The Cold Svalt gate opened before Dusty Ruins was complete.");
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
    return testColdSvaltRoute(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
