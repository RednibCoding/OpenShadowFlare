#include "episode_one_test_support.hpp"
#include "world/retail_save_file.hpp"
#include "world/retail_save_progress.hpp"
#include "world/retail_save_world_state.hpp"
#include "world/world_scene.hpp"

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
    std::int32_t route_state) {
    osf::RetailSaveProgress progress = seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    for (const std::int32_t completed :
         {0, 1, 2, 3, 4, 6, 7, 8, 9, 10}) {
        progress.quest_flags[
            static_cast<std::size_t>(completed)] = 2;
    }
    if (progress.script_state_flags.size() < 72) {
        progress.script_state_flags.resize(72, 0);
    }
    progress.script_state_flags[11] = 2;
    progress.script_state_flags[15] = 1;
    progress.script_state_flags[71] = route_state;
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
        0x4c,
        &error);
}

bool walkTowardPoint(
    osf::WorldScene& world,
    osf::WorldPosition target,
    std::int32_t maximum_updates) {
    const std::int32_t source_scenario = world.scenarioId();
    for (std::int32_t update = 0;
         update < maximum_updates;
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
        if (world.scenarioId() != source_scenario) {
            return false;
        }
    }
    return true;
}

bool atScenarioObject(
    const osf::WorldScene& world,
    const osf::ScenarioObjectActor& object,
    std::int32_t tolerance) {
    const osf::ObjectBounds& bounds = object.judgement();
    return world.playerWorldX() >=
               object.position().x + bounds.left - tolerance &&
           world.playerWorldX() <=
               object.position().x + bounds.right + tolerance &&
           world.playerWorldY() >=
               object.position().y + bounds.top - tolerance &&
           world.playerWorldY() <=
               object.position().y + bounds.bottom + tolerance;
}

bool changedOnce(osf::WorldScene& world) {
    return world.takeScenarioChanged() &&
           !world.takeScenarioChanged();
}

bool populatedVendor(
    const osf::WorldScene& world,
    std::int32_t index) {
    const osf::VendorInventory* vendor =
        world.vendorInventory(index);
    return vendor && !vendor->items().empty();
}

bool hasNpc(
    const osf::WorldScene& world,
    std::int32_t id,
    const std::string& name) {
    for (const osf::NpcActor& npc : world.npcs()) {
        if (npc.id() == id && npc.name() == name) {
            return true;
        }
    }
    return false;
}

bool testMiningTownRoute(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "MiningRoute";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The Mining Town route fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerData level_sixty = seed.playerData();
    if (!check(
            raiseToLevel(level_sixty, 60, seed.parameterTables()),
            "The Mining Town route fixture could not reach its area level.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_two_mining_town_route_test";
    const std::filesystem::path locked_save =
        fixture_root / "locked" / "Save" / "0000.Ssv";
    const std::filesystem::path route_save =
        fixture_root / "route" / "Save" / "0000.Ssv";
    const std::filesystem::path mining_save =
        fixture_root / "mining" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);

    if (!check(
            writeFixture(
                locked_save,
                seed,
                level_sixty,
                routeProgress(seed, 0),
                {true, 1, 4},
                error) &&
                writeFixture(
                    route_save,
                    seed,
                    level_sixty,
                    routeProgress(seed, 1),
                    {true, 1, 4},
                    error),
            "The locked or open caravan route fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene locked;
    if (!check(
            loadSavedFixture(data_root, locked_save, locked, error) &&
                locked.scenarioId() == 1,
            "The locked caravan route fixture could not be restored.")) {
        std::cerr << error << '\n';
        return false;
    }
    locked.update();
    const osf::ScenarioObjectActor* locked_route_trigger =
        osf::test::findScenarioTrigger(locked, 4);
    const osf::WorldPosition locked_route_target =
        locked_route_trigger
            ? osf::test::scenarioTriggerCenter(
                  *locked_route_trigger)
            : osf::WorldPosition{};
    if (!check(
            locked_route_trigger &&
                osf::test::scriptedObjectVisible(
                locked, 10001030, true) &&
                osf::test::scriptedObjectVisible(
                    locked, 10001031, false) &&
                walkTowardPoint(
                    locked,
                    locked_route_target,
                    600) &&
                atScenarioObject(
                    locked, *locked_route_trigger, 50) &&
                locked.scenarioId() == 1,
            "Near Remote Town exposed the caravan route before flag 71.")) {
        return false;
    }

    osf::WorldScene route;
    if (!check(
            loadSavedFixture(data_root, route_save, route, error) &&
                route.scenarioId() == 1,
            "The open caravan route fixture could not be restored.")) {
        std::cerr << error << '\n';
        return false;
    }
    route.update();
    if (!check(
            osf::test::scriptedObjectVisible(
                route, 10001030, false) &&
                osf::test::scriptedObjectVisible(
                    route, 10001031, true) &&
                osf::test::walkThroughScenarioTrigger(
                    route, 4, 2999999) &&
                changedOnce(route) &&
                route.scenario().title() == "Caravan" &&
                route.retailSaveWorldState().entry_value == 0 &&
                !route.scenarioVisualActive() &&
                route.retailSaveProgress().script_state_flags[71] == 1,
            "The authored caravan gate did not enter Caravan cleanly.")) {
        std::cerr << "scenario=" << route.scenarioId()
                  << " title=" << route.scenario().title()
                  << " entry="
                  << route.retailSaveWorldState().entry_value
                  << " visual=" << route.scenarioVisualActive()
                  << '\n';
        return false;
    }

    if (!check(
            route.transitionScenario(
                {2000000, 0, 0}, &error) ==
                    osf::ScenarioTravelResult::loaded &&
                changedOnce(route) &&
                route.retailSaveWorldState().entry_value == 0 &&
                route.scenario().title() == "Forest" &&
                route.musicTrack() == 1 &&
                !route.scenarioVisualActive(),
            "Caravan did not enter the authored Episode 2 road.")) {
        std::cerr << "scenario=" << route.scenarioId()
                  << " title=" << route.scenario().title()
                  << " player=" << route.playerWorldX() << ','
                  << route.playerWorldY() << '\n';
        for (const osf::ScenarioObjectActor& object :
             route.scenarioObjects()) {
            if (object.id() != 1) {
                continue;
            }
            const osf::ObjectBounds& bounds = object.judgement();
            std::cerr << "trigger=" << object.position().x << ','
                      << object.position().y << " bounds="
                      << bounds.left << ',' << bounds.top << ','
                      << bounds.right << ',' << bounds.bottom
                      << " state=" << object.visible() << ','
                      << object.pointerEnabled() << ','
                      << object.judgementEnabled() << '\n';
        }
        for (const osf::NpcActor& npc : route.npcs()) {
            const osf::ObjectBounds& bounds = npc.judgement();
            std::cerr << "npc=" << npc.id() << ' ' << npc.name()
                      << ' ' << npc.position().x << ','
                      << npc.position().y << " bounds="
                      << bounds.left << ',' << bounds.top << ','
                      << bounds.right << ',' << bounds.bottom << '\n';
        }
        return false;
    }
    if (!check(
            route.transitionScenario(
                {2000000, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 1, 2000001) &&
                changedOnce(route) &&
                route.retailSaveWorldState().entry_value == 0 &&
                route.scenario().title() == "Forest" &&
                route.musicTrack() == 1 &&
                !route.scenarioVisualActive(),
            "The first Episode 2 road map did not reach its second half.")) {
        std::cerr << "scenario=" << route.scenarioId()
                  << " title=" << route.scenario().title() << '\n';
        return false;
    }
    if (!check(
            route.transitionScenario(
                {2000001, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 1, 2100000) &&
                changedOnce(route) &&
                route.scenario().title() ==
                    "Kanfore, Mining Town" &&
                route.retailSaveWorldState().entry_value == 0 &&
                route.npcs().size() == 14 &&
                hasNpc(route, 100, "Beboba") &&
                populatedVendor(route, 0) &&
                populatedVendor(route, 1) &&
                populatedVendor(route, 2) &&
                !route.scenarioVisualActive() &&
                route.retailSaveProgress().script_state_flags[71] == 1,
            "The authored road did not enter a fully initialized Mining Town.")) {
        std::cerr << "final=" << route.scenarioId() << '/'
                  << route.scenario().title()
                  << " entry="
                  << route.retailSaveWorldState().entry_value
                  << " people=" << route.npcs().size()
                  << " vendors=" << populatedVendor(route, 0) << ','
                  << populatedVendor(route, 1) << ','
                  << populatedVendor(route, 2) << '\n';
        return false;
    }

    if (!check(
            writeFixture(
                mining_save,
                route,
                route.playerData(),
                route.retailSaveProgress(),
                route.retailSaveWorldState(),
                error),
            "Mining Town could not be saved through the retail owner.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene persisted;
    const bool persisted_route =
        loadSavedFixture(data_root, mining_save, persisted, error) &&
        persisted.scenarioId() == 2100000 &&
        persisted.scenario().title() == "Kanfore, Mining Town" &&
        persisted.retailSaveWorldState().entry_value == 0 &&
        persisted.retailSaveProgress().script_state_flags[71] == 1 &&
        populatedVendor(persisted, 0) &&
        populatedVendor(persisted, 1) &&
        populatedVendor(persisted, 2) &&
        osf::test::walkThroughScenarioTrigger(
            persisted, 0, 2000001) &&
        persisted.retailSaveWorldState().entry_value == 1;
    std::filesystem::remove_all(fixture_root, cleanup_error);
    return check(
        persisted_route,
        "Saving Mining Town lost the route flag, services, or return exit.");
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "02100000")) {
        return 0;
    }
    return testMiningTownRoute(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
