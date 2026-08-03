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
#include <vector>

namespace {

using osf::test::check;
using osf::test::containsSample;
using osf::test::loadSavedFixture;
using osf::test::raiseToLevel;

osf::RetailSaveProgress postDragonProgress(
    const osf::WorldScene& seed) {
    osf::RetailSaveProgress progress = seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    for (const std::int32_t completed :
         {0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 13, 14, 15, 16, 17}) {
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
    progress.script_state_flags[38] = 1;
    progress.script_state_flags[39] = 2;
    progress.script_state_flags[40] = 1;
    progress.script_state_flags[41] = 1;
    progress.script_state_flags[45] = 1;
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
        0x58,
        &error);
}

std::int32_t groundGold(const osf::WorldScene& world) {
    std::int32_t gold = 0;
    for (const osf::GroundItem& item : world.groundItems()) {
        if (item.item.category == 4 && item.item.definition_id == 0) {
            gold += item.item.quantity;
        }
    }
    return gold;
}

bool advanceMessages(
    osf::WorldScene& world,
    std::int32_t first,
    std::int32_t last,
    const char* failure) {
    for (std::int32_t message = first; message <= last; ++message) {
        world.advanceConversation();
        if (!check(world.conversationMessageId() == message, failure)) {
            std::cerr << "message=" << world.conversationMessageId()
                      << " expected=" << message << '\n';
            return false;
        }
    }
    return true;
}

bool testCentralFront(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "CentralFront";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The central-front fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerData level_sixty = seed.playerData();
    if (!check(
            raiseToLevel(level_sixty, 60, seed.parameterTables()),
            "The central-front fixture could not reach its area level.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_two_central_front_test";
    const std::filesystem::path dragon_save =
        fixture_root / "dragon" / "Save" / "0000.Ssv";
    const std::filesystem::path report_save =
        fixture_root / "report" / "Save" / "0000.Ssv";
    const std::filesystem::path kyle_save =
        fixture_root / "kyle" / "Save" / "0000.Ssv";
    const std::filesystem::path camp_save =
        fixture_root / "camp" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);

    if (!check(
            writeFixture(
                dragon_save,
                seed,
                level_sixty,
                postDragonProgress(seed),
                {true, 2200000, 0},
                error),
            "The post-dragon Fanann fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene fanann;
    if (!check(
            loadSavedFixture(data_root, dragon_save, fanann, error) &&
                fanann.quests().state(12) == 1 &&
                fanann.quests().state(17) == 2 &&
                osf::test::openNpcConversation(fanann, 0) &&
                fanann.conversationMessageId() == 1000018 &&
                fanann.retailSaveProgress()
                        .script_state_flags[41] == 2,
            "Lytle did not begin the complete dragon-victory report.")) {
        std::cerr << error << " message="
                  << fanann.conversationMessageId() << '\n';
        return false;
    }
    if (!advanceMessages(
            fanann,
            1000019,
            1000025,
            "Lytle's dragon-victory report skipped a message.")) {
        return false;
    }
    fanann.advanceConversation();
    fanann.advanceConversation();
    if (!check(
            !fanann.conversationActive() &&
                fanann.retailSaveProgress()
                        .script_state_flags[41] == 2 &&
                !fanann.transports().enabled(25),
            "Lytle's first report unlocked the central front too early.")) {
        return false;
    }

    if (!check(
            writeFixture(
                report_save,
                fanann,
                fanann.playerData(),
                fanann.retailSaveProgress(),
                fanann.retailSaveWorldState(),
                error),
            "Lytle's completed dragon report could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene repeated;
    if (!check(
            loadSavedFixture(data_root, report_save, repeated, error) &&
                osf::test::openNpcConversation(repeated, 0) &&
                repeated.conversationMessageId() == 1000026,
            "Lytle did not use the authored central-front reminder.")) {
        std::cerr << error << " message="
                  << repeated.conversationMessageId() << '\n';
        return false;
    }
    repeated.advanceConversation();
    if (!check(
            repeated.conversationMessageId() == 1000027,
            "Lytle's central-front reminder skipped its second message.")) {
        return false;
    }
    repeated.advanceConversation();
    repeated.advanceConversation();

    if (!check(
            repeated.transitionScenario({2100000, 0, 0}, &error) ==
                    osf::ScenarioTravelResult::loaded &&
                repeated.scenario().title() ==
                    "Kanfore, Mining Town" &&
                osf::test::openNpcConversation(repeated, 0) &&
                repeated.conversationMessageId() == 1000014 &&
                repeated.quests().state(12) == 2 &&
                repeated.groundItems().size() == 4 &&
                groundGold(repeated) == 40000,
            "Kyle did not complete the Yugunos report with 40,000 Gold.")) {
        std::cerr << error << " message="
                  << repeated.conversationMessageId()
                  << " items=" << repeated.groundItems().size()
                  << " gold=" << groundGold(repeated) << '\n';
        return false;
    }
    std::vector<std::int32_t> report_audio =
        repeated.takeAudioSamples();
    for (std::int32_t update = 0; update < 19; ++update) {
        repeated.update();
    }
    const std::vector<std::int32_t> landing_audio =
        repeated.takeAudioSamples();
    report_audio.insert(
        report_audio.end(), landing_audio.begin(), landing_audio.end());
    if (!check(
            !containsSample(report_audio, 66) &&
                std::count(
                    report_audio.begin(), report_audio.end(), 85) == 4,
            "Kyle's silent quest update or Gold landing sounds differed.")) {
        std::cerr << "sample66="
                  << std::count(
                         report_audio.begin(), report_audio.end(), 66)
                  << " sample85="
                  << std::count(
                         report_audio.begin(), report_audio.end(), 85)
                  << '\n';
        return false;
    }
    if (!advanceMessages(
            repeated,
            1000015,
            1000017,
            "Kyle's Yugunos report skipped a message.")) {
        return false;
    }
    repeated.advanceConversation();
    repeated.advanceConversation();
    if (!check(
            osf::test::openNpcConversation(repeated, 0) &&
                repeated.conversationMessageId() == 1000018 &&
                repeated.groundItems().size() == 4 &&
                groundGold(repeated) == 40000,
            "Kyle's completed branch repeated the Gold reward.")) {
        return false;
    }
    repeated.advanceConversation();
    if (!check(
            repeated.conversationMessageId() == 1000019,
            "Kyle's completed branch skipped its final message.")) {
        return false;
    }
    repeated.advanceConversation();
    repeated.advanceConversation();

    if (!check(
            writeFixture(
                kyle_save,
                repeated,
                repeated.playerData(),
                repeated.retailSaveProgress(),
                {true, 2200000, 0},
                error),
            "Kyle's completed Yugunos report could not return to Fanann.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene unlocked;
    if (!check(
            loadSavedFixture(data_root, kyle_save, unlocked, error) &&
                unlocked.quests().state(12) == 2 &&
                unlocked.groundItems().empty() &&
                osf::test::openNpcConversation(unlocked, 0) &&
                unlocked.conversationMessageId() == 1000028 &&
                unlocked.retailSaveProgress()
                        .script_state_flags[41] == 4 &&
                unlocked.transports().enabled(25),
            "Lytle did not unlock the South Camp transport.")) {
        std::cerr << error << " message="
                  << unlocked.conversationMessageId() << '\n';
        return false;
    }
    if (!advanceMessages(
            unlocked,
            1000029,
            1000029,
            "Lytle's South Camp directions skipped a message.")) {
        return false;
    }
    unlocked.advanceConversation();
    unlocked.advanceConversation();
    if (!check(
            osf::test::openNpcConversation(unlocked, 0) &&
                unlocked.conversationMessageId() == 1000030 &&
                unlocked.retailSaveProgress()
                        .script_state_flags[41] == 4 &&
                unlocked.transports().enabled(25),
            "Lytle's unlocked South Camp branch did not persist.")) {
        return false;
    }
    unlocked.advanceConversation();
    unlocked.advanceConversation();

    if (!check(
            writeFixture(
                camp_save,
                unlocked,
                unlocked.playerData(),
                unlocked.retailSaveProgress(),
                unlocked.retailSaveWorldState(),
                error),
            "The South Camp transport unlock could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene persisted;
    const osf::TransportDestination* south_camp = nullptr;
    if (!check(
            loadSavedFixture(data_root, camp_save, persisted, error) &&
                persisted.retailSaveProgress()
                        .script_state_flags[41] == 4 &&
                persisted.transports().enabled(25) &&
                (south_camp = persisted.transports().find(25)) &&
                south_camp->name == "South Camp of Yugunos" &&
                south_camp->scenario == 3900000 &&
                south_camp->entry == 50 &&
                persisted.activateTransportDestination(25, &error) ==
                    osf::ScenarioTravelResult::loaded &&
                persisted.scenario().title() ==
                    "South Camp of Yugunos ",
            "The saved transport did not reach South Camp of Yugunos.")) {
        std::cerr << error << " scenario=" << persisted.scenarioId()
                  << " title=" << persisted.scenario().title() << '\n';
        return false;
    }

    std::filesystem::remove_all(fixture_root, cleanup_error);
    return true;
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "03900000")) {
        return 0;
    }
    return testCentralFront(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
