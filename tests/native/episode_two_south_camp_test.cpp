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

osf::RetailSaveProgress southCampProgress(
    const osf::WorldScene& seed) {
    osf::RetailSaveProgress progress = seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    for (const std::int32_t completed : {
             0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13, 14,
             15, 16, 17}) {
        progress.quest_flags[
            static_cast<std::size_t>(completed)] = 2;
    }
    if (progress.script_state_flags.size() < 78) {
        progress.script_state_flags.resize(78, 0);
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
        0x59,
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

bool authoredSweepTargets(const osf::WorldScene& world) {
    const auto flame = std::find_if(
        world.enemies().begin(),
        world.enemies().end(),
        [](const osf::EnemyActor& enemy) {
            return enemy.id() == 20000 &&
                   enemy.name() == "Flame Warrior" &&
                   enemy.maximumLife() == 30000;
        });
    const auto dread = std::find_if(
        world.enemies().begin(),
        world.enemies().end(),
        [](const osf::EnemyActor& enemy) {
            return enemy.id() == 20001 &&
                   enemy.name() == "Dread Warrior" &&
                   enemy.maximumLife() == 30000;
        });
    return world.enemies().size() == 422 &&
           flame != world.enemies().end() &&
           dread != world.enemies().end();
}

bool testSouthCampSweep(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "SouthCamp";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, new_player, &error),
            "The South Camp fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerData level_sixty = seed.playerData();
    if (!check(
            raiseToLevel(level_sixty, 60, seed.parameterTables()),
            "The South Camp fixture could not reach its area level.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_two_south_camp_test";
    const std::filesystem::path arrival_save =
        fixture_root / "arrival" / "Save" / "0000.Ssv";
    const std::filesystem::path completed_save =
        fixture_root / "completed" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);

    if (!check(
            writeFixture(
                arrival_save,
                seed,
                level_sixty,
                southCampProgress(seed),
                {true, 3900000, 50},
                error),
            "The South Camp arrival fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene camp;
    if (!check(
            loadSavedFixture(data_root, arrival_save, camp, error) &&
                camp.scenarioId() == 3900000 &&
                camp.scenario().title() ==
                    "South Camp of Yugunos " &&
                camp.quests().state(20) == 0,
            "The saved route did not arrive in South Camp.")) {
        std::cerr << error << '\n';
        return false;
    }
    camp.update();
    if (!check(
            camp.scenarioVisualActive() &&
                camp.scenarioVisual().visualId() == 3,
            "South Camp did not open its one-time Visual03 briefing.")) {
        std::cerr << error << " visual="
                  << camp.scenarioVisualActive() << '\n';
        return false;
    }
    for (std::int32_t frame = 0; frame < 300; ++frame) {
        camp.advanceScenarioVisualFrame();
    }
    camp.requestScenarioVisualAdvance();
    camp.advanceScenarioVisualFrame();
    camp.advanceScenarioVisualFrame();
    if (!check(
            !camp.scenarioVisualActive() &&
                camp.retailSaveProgress()
                        .script_state_flags[104] == 1 &&
                osf::test::openNpcConversation(camp, 0) &&
                camp.conversationMessageId() == 1000002,
            "Jeel did not begin the authored South Camp introduction.")) {
        std::cerr << error << " message="
                  << camp.conversationMessageId()
                  << " scenario=" << camp.scenarioId()
                  << " title=" << camp.scenario().title()
                  << " quest=" << camp.quests().state(20)
                  << " npcs=" << camp.npcs().size()
                  << " player=" << camp.playerRenderPosition(1.0).x
                  << ',' << camp.playerRenderPosition(1.0).y
                  << " life=" << camp.playerCurrentLife()
                  << " walk="
                  << camp.playerRuntimeProfile().walking_speed_raw
                  << '/' << camp.playerRuntimeProfile().walkingSpeedTier()
                  << " motion="
                  << static_cast<std::int32_t>(camp.playerMotion())
                  << " pending=" << camp.interactionPending() << '\n';
        std::cerr << "visual=" << camp.scenarioVisualActive()
                  << " conversation=" << camp.conversationActive()
                  << '\n';
        return false;
    }
    if (!advanceMessages(
            camp,
            {1000003, 1000004},
            "Jeel's South Camp introduction skipped a message.")) {
        return false;
    }
    camp.advanceConversation();
    if (!check(
            !camp.conversationActive() &&
                osf::test::openNpcConversation(camp, 0) &&
                camp.conversationMessageId() == 1000005,
            "Jeel did not continue from his introduction into the sweep briefing.")) {
        std::cerr << "message=" << camp.conversationMessageId()
                  << '\n';
        return false;
    }
    if (!advanceMessages(
            camp,
            {1000006},
            "Jeel's sweep briefing skipped a message.")) {
        return false;
    }
    const std::vector<std::int32_t> offer_audio =
        camp.takeAudioSamples();
    const osf::MissionDefinition* mission =
        camp.missions().find(20);
    if (!check(
            mission && mission->title ==
                           "Sweep vicinity of S. Camp of Yugunos." &&
                camp.quests().state(20) == 1 &&
                camp.quests().lastCue() == osf::QuestCue::updated &&
                camp.quests().notice().quest_id == 20 &&
                containsSample(offer_audio, 65) &&
                camp.retailSaveProgress().script_state_flags[74] == 1,
            "Jeel did not start mission twenty with its notice and cue.")) {
        return false;
    }
    camp.advanceConversation();
    if (!check(
            !camp.conversationActive() &&
                osf::test::openNpcConversation(camp, 0) &&
                camp.conversationMessageId() == 1000007,
            "Jeel did not use the active sweep reminder.")) {
        std::cerr << "message=" << camp.conversationMessageId()
                  << '\n';
        return false;
    }
    camp.advanceConversation();
    camp.advanceConversation();

    if (!check(
            camp.transitionScenario({3900000, 0, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    camp, 0, 3000507) &&
                camp.scenario().title() == "East Antalusia" &&
                camp.retailSaveWorldState().entry_value == 0 &&
                camp.transitionScenario({3000507, 1, 0}, &error) ==
                    osf::ScenarioTravelResult::relocated &&
                osf::test::walkThroughScenarioTrigger(
                    camp, 1, 3000407) &&
                camp.scenario().title() ==
                    "The Foot of Mt. Tedoron" &&
                camp.retailSaveWorldState().entry_value == 0,
            "South Camp's authored north route did not reach Mt. Tedoron.")) {
        std::cerr << error << " scenario=" << camp.scenarioId()
                  << " title=" << camp.scenario().title()
                  << " entry="
                  << camp.retailSaveWorldState().entry_value << '\n';
        return false;
    }
    if (!check(
            camp.quests().state(20) == 1 &&
                authoredSweepTargets(camp) &&
                osf::test::markScenarioEnemiesDefeated(
                    camp, 20000, 20001),
            "The authored Mt. Tedoron sweep targets differ from retail.")) {
        return false;
    }

    std::vector<std::int32_t> completion_audio;
    for (std::int32_t update = 0;
         update < 400 && camp.quests().state(20) != 2;
         ++update) {
        camp.update();
        const std::vector<std::int32_t> samples =
            camp.takeAudioSamples();
        completion_audio.insert(
            completion_audio.end(), samples.begin(), samples.end());
    }
    if (!check(
            camp.quests().state(20) == 2 &&
                camp.quests().lastCue() == osf::QuestCue::completed &&
                containsSample(completion_audio, 66),
            "The two-warrior clear did not complete mission twenty.")) {
        return false;
    }

    if (!check(
            writeFixture(
                completed_save,
                camp,
                camp.playerData(),
                camp.retailSaveProgress(),
                {true, 3900000, 50},
                error),
            "The completed sweep could not be saved for Jeel's report.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene reported;
    const std::int32_t experience_before =
        camp.playerData().experience();
    std::vector<std::int32_t> report_audio;
    if (!check(
            loadSavedFixture(
                data_root, completed_save, reported, error) &&
                reported.quests().state(20) == 2 &&
                reported.retailSaveProgress()
                        .script_state_flags[104] == 1 &&
                osf::test::openNpcConversation(
                    reported, 0, &report_audio) &&
                reported.conversationMessageId() == 1000008 &&
                reported.retailSaveProgress()
                        .script_state_flags[74] == 2 &&
                reported.playerData().experience() > experience_before,
            "Jeel did not accept the completed sweep exactly once.")) {
        std::cerr << error << " message="
                  << reported.conversationMessageId() << '\n';
        return false;
    }
    const std::int32_t rewarded_experience =
        reported.playerData().experience();
    const std::vector<std::int32_t> queued_report_audio =
        reported.takeAudioSamples();
    report_audio.insert(
        report_audio.end(),
        queued_report_audio.begin(),
        queued_report_audio.end());
    if (!check(
            containsSample(report_audio, 64),
            "Jeel's sweep report did not play its authored reward sound.")) {
        return false;
    }
    if (!advanceMessages(
            reported,
            {1000009, 1000010, 1000011},
            "Jeel's post-sweep handoff skipped a message.")) {
        return false;
    }
    reported.advanceConversation();
    std::vector<std::int32_t> repeat_audio;
    if (!check(
            !reported.conversationActive() &&
                osf::test::openNpcConversation(
                    reported, 0, &repeat_audio) &&
                reported.conversationMessageId() == 1000012 &&
                reported.retailSaveProgress()
                        .script_state_flags[74] == 2 &&
                reported.playerData().experience() ==
                    rewarded_experience &&
                !containsSample(repeat_audio, 64),
            "Jeel repeated the sweep reward instead of its handoff.")) {
        std::cerr << "message=" << reported.conversationMessageId()
                  << '\n';
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
            data_root / "Scenario" / "03900000") ||
        !std::filesystem::is_directory(
            data_root / "Scenario" / "03000407")) {
        return 0;
    }
    return testSouthCampSweep(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
