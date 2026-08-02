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

osf::RetailSaveProgress fanannProgress(const osf::WorldScene& seed) {
    osf::RetailSaveProgress progress = seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    for (const std::int32_t completed :
         {0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 13, 14}) {
        progress.quest_flags[
            static_cast<std::size_t>(completed)] = 2;
    }
    progress.quest_flags[12] = 1;
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
        0x52,
        &error);
}

bool populatedVendor(const osf::WorldScene& world, std::int32_t index) {
    const osf::VendorInventory* vendor = world.vendorInventory(index);
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

bool testFanannHandoff(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "FanannHandoff";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The Fanann fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerData level_sixty = seed.playerData();
    if (!check(
            raiseToLevel(level_sixty, 60, seed.parameterTables()),
            "The Fanann fixture could not reach its area level.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_two_fanann_handoff_test";
    const std::filesystem::path route_save =
        fixture_root / "route" / "Save" / "0000.Ssv";
    const std::filesystem::path greeted_save =
        fixture_root / "greeted" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);

    if (!check(
            writeFixture(
                route_save,
                seed,
                level_sixty,
                fanannProgress(seed),
                {true, 2100004, 0},
                error),
            "The open Fanann route fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene fanann;
    if (!check(
            loadSavedFixture(data_root, route_save, fanann, error) &&
                fanann.transitionScenario(
                    {2100004, 3, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    fanann, 3, 2200000) &&
                fanann.scenario().title() ==
                    "Fanann, Village of Elves" &&
                fanann.retailSaveWorldState().entry_value == 0 &&
                hasNpc(fanann, 0, "Lytle") &&
                populatedVendor(fanann, 0) &&
                populatedVendor(fanann, 1) &&
                populatedVendor(fanann, 2),
            "The opened gate did not enter a fully initialized Fanann.")) {
        std::cerr << error << '\n';
        return false;
    }

    if (!check(
            osf::test::openNpcConversation(fanann, 0) &&
                fanann.conversationMessageId() == 1000002 &&
                fanann.retailSaveProgress().script_state_flags[41] == 1 &&
                fanann.quests().state(12) == 1 &&
                fanann.quests().state(14) == 2,
            "Lytle did not acknowledge the Yugunos investigation once.")) {
        std::cerr << "message=" << fanann.conversationMessageId()
                  << " flag41="
                  << fanann.retailSaveProgress().script_state_flags[41]
                  << '\n';
        return false;
    }
    for (const std::int32_t message : {1000003, 1000004}) {
        fanann.advanceConversation();
        if (!check(
                fanann.conversationMessageId() == message,
                "Lytle's Yugunos directions skipped a message.")) {
            std::cerr << "message=" << fanann.conversationMessageId()
                      << " expected=" << message << '\n';
            return false;
        }
    }
    fanann.advanceConversation();
    if (!check(
            !fanann.conversationActive(),
            "Lytle did not release the initial Fanann greeting.")) {
        return false;
    }

    if (!check(
            writeFixture(
                greeted_save,
                fanann,
                fanann.playerData(),
                fanann.retailSaveProgress(),
                fanann.retailSaveWorldState(),
                error),
            "The Fanann greeting could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene persisted;
    const bool handoff_persisted =
        loadSavedFixture(data_root, greeted_save, persisted, error) &&
        persisted.scenarioId() == 2200000 &&
        persisted.retailSaveProgress().script_state_flags[41] == 1 &&
        persisted.quests().state(12) == 1 &&
        persisted.quests().state(14) == 2 &&
        osf::test::openNpcConversation(persisted, 0) &&
        persisted.conversationMessageId() == 1000006;
    if (persisted.conversationActive()) {
        persisted.advanceConversation();
        persisted.advanceConversation();
    }
    const bool yugunos_route =
        handoff_persisted &&
        persisted.transitionScenario(
            {2200000, 1, 0}, &error) ==
            osf::ScenarioTravelResult::relocated &&
        osf::test::walkThroughScenarioTrigger(
            persisted, 1, 2200001) &&
        persisted.scenario().title() == "Butterfly Hill" &&
        persisted.retailSaveWorldState().entry_value == 0 &&
        persisted.retailSaveProgress().script_state_flags[41] == 1 &&
        persisted.quests().state(12) == 1 &&
        persisted.transitionScenario(
            {2200001, 1, 0}, &error) ==
            osf::ScenarioTravelResult::relocated &&
        osf::test::walkThroughScenarioTrigger(
            persisted, 1, 2200003) &&
        persisted.scenario().title() == "Dragon Road" &&
        persisted.retailSaveWorldState().entry_value == 0 &&
        persisted.retailSaveProgress().script_state_flags[41] == 1 &&
        persisted.quests().state(12) == 1;

    std::filesystem::remove_all(fixture_root, cleanup_error);
    return check(
        yugunos_route,
        "Saving Lytle's handoff lost its branch or Yugunos route.");
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "02200000")) {
        return 0;
    }
    return testFanannHandoff(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
