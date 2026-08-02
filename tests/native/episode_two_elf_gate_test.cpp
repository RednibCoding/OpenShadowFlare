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

osf::RetailSaveProgress miningTunnelProgress(
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
    if (progress.script_state_flags.size() < 72) {
        progress.script_state_flags.resize(72, 0);
    }
    progress.script_state_flags[11] = 2;
    progress.script_state_flags[15] = 1;
    progress.script_state_flags[23] = 1;
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
        0x4e,
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

bool testElfGate(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "ElfGate";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The elf-gate fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerData level_sixty = seed.playerData();
    if (!check(
            raiseToLevel(level_sixty, 60, seed.parameterTables()),
            "The elf-gate fixture could not reach its area level.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_two_elf_gate_test";
    const std::filesystem::path route_save =
        fixture_root / "route" / "Save" / "0000.Ssv";
    const std::filesystem::path return_save =
        fixture_root / "return" / "Save" / "0000.Ssv";
    const std::filesystem::path persisted_save =
        fixture_root / "persisted" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);

    if (!check(
            writeFixture(
                route_save,
                seed,
                level_sixty,
                miningTunnelProgress(seed),
                {true, 2100000, 0},
                error),
            "The active mining-tunnel fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene route;
    if (!check(
            loadSavedFixture(data_root, route_save, route, error) &&
                route.quests().state(12) == 1 &&
                osf::test::openNpcConversation(route, 0) &&
                route.conversationMessageId() == 1000013,
            "Kyle did not retain the active mining-tunnel branch.")) {
        std::cerr << error << '\n';
        return false;
    }
    route.advanceConversation();

    if (!check(
            route.transitionScenario(
                {2100000, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    route, 1, 2100001) &&
                route.scenario().title() ==
                    "Forest of Four Leaves" &&
                route.transitionScenario(
                    {2100004, 0, 0}, &error) ==
                    osf::ScenarioTravelResult::loaded &&
                route.scenario().title() == "Cross Agora" &&
                route.retailSaveWorldState().entry_value == 0,
            "The authored route from Mining Town did not reach Cross Agora.")) {
        std::cerr << "scenario=" << route.scenarioId()
                  << " title=" << route.scenario().title()
                  << " entry="
                  << route.retailSaveWorldState().entry_value
                  << '\n';
        return false;
    }

    if (!check(
            route.transitionScenario(
                {2100004, 3, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::openNpcConversation(route, 0) &&
                route.conversationMessageId() == 1000003,
            "Cross Agora's elf did not refuse the first visit.")) {
        std::cerr << error << '\n';
        return false;
    }
    route.advanceConversation();
    if (!check(
            route.conversationMessageId() == 1000004,
            "Garshwin skipped the final refusal message.")) {
        return false;
    }
    route.advanceConversation();
    if (!check(
            !route.conversationActive() &&
                route.retailSaveProgress().script_state_flags[24] == 1 &&
                route.quests().state(12) == 1 &&
                route.quests().state(13) == 0,
            "The elf refusal did not latch the authored return branch.")) {
        return false;
    }

    if (!check(
            route.transitionScenario(
                {2100004, 3, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated,
            "The Cross Agora gate fixture could not use its southern entry.")) {
        return false;
    }
    const osf::ScenarioObjectActor* elf_gate =
        osf::test::findScenarioTrigger(route, 3);
    const osf::WorldPosition gate_target =
        elf_gate
            ? osf::test::scenarioTriggerCenter(*elf_gate)
            : osf::WorldPosition{};
    if (!check(
            elf_gate &&
                walkTowardPoint(route, gate_target, 600) &&
                atScenarioObject(route, *elf_gate, 50) &&
                route.scenarioId() == 2100004,
            "Cross Agora allowed passage before the elf mission gate.")) {
        return false;
    }

    if (!check(
            writeFixture(
                return_save,
                route,
                route.playerData(),
                route.retailSaveProgress(),
                {true, 2100000, 0},
                error),
            "The refused elf-gate state could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene returned;
    if (!check(
            loadSavedFixture(data_root, return_save, returned, error) &&
                osf::test::openNpcConversation(returned, 0) &&
                returned.conversationMessageId() == 1000020,
            "Kyle did not react to the refused Forest of Elves gate.")) {
        std::cerr << error << '\n';
        return false;
    }
    for (const std::int32_t message :
         {1000021, 1000022, 1000023, 1000024,
          1000025, 1000026, 1000027, 1000028, 1000029}) {
        returned.advanceConversation();
        if (!check(
                returned.conversationMessageId() == message,
                "Kyle's Kirushutat briefing skipped a message.")) {
            std::cerr << "message="
                      << returned.conversationMessageId()
                      << " expected=" << message << '\n';
            return false;
        }
    }
    const std::vector<std::int32_t> offer_audio =
        returned.takeAudioSamples();
    const osf::MissionDefinition* mission =
        returned.missions().find(13);
    if (!check(
            mission &&
                mission->title ==
                    "Meet with the Wizard Kirushutat." &&
                returned.quests().state(12) == 1 &&
                returned.quests().state(13) == 1 &&
                returned.quests().notice().quest_id == 13 &&
                returned.quests().notice().counter == 600 &&
                containsSample(offer_audio, 65),
            "Kyle did not start the authored Kirushutat mission.")) {
        return false;
    }
    returned.advanceConversation();
    if (!check(
            !returned.conversationActive(),
            "Kyle did not release the Kirushutat briefing.")) {
        return false;
    }

    if (!check(
            writeFixture(
                persisted_save,
                returned,
                returned.playerData(),
                returned.retailSaveProgress(),
                {true, 2100000, 0},
                error),
            "The Kirushutat mission state could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene persisted;
    const bool no_repeat =
        loadSavedFixture(
            data_root, persisted_save, persisted, error) &&
        persisted.quests().state(12) == 1 &&
        persisted.quests().state(13) == 1 &&
        persisted.retailSaveProgress().script_state_flags[24] == 1 &&
        osf::test::openNpcConversation(persisted, 0) &&
        persisted.conversationMessageId() == 1000013 &&
        persisted.groundItems().empty();
    if (persisted.conversationActive()) {
        persisted.advanceConversation();
    }
    std::filesystem::remove_all(fixture_root, cleanup_error);
    return check(
        no_repeat,
        "Saving the Kirushutat handoff repeated the gate briefing.");
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "02100004")) {
        return 0;
    }
    return testElfGate(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
