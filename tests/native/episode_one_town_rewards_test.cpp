#include "episode_one_test_support.hpp"
#include "world/retail_save_file.hpp"
#include "world/retail_save_progress.hpp"
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

osf::RetailSaveProgress postDustyRuinsProgress(
    const osf::WorldScene& seed) {
    osf::RetailSaveProgress progress =
        seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    progress.quest_flags[0] = 2;
    progress.quest_flags[1] = 2;
    progress.quest_flags[2] = 2;
    progress.quest_flags[3] = 2;
    if (progress.script_state_flags.size() < 72) {
        progress.script_state_flags.resize(72, 0);
    }
    // These are the authored Remote Town conversation latches reached by a
    // normal playthrough before the post-Dusty Ruins callbacks.
    progress.script_state_flags[2] = 1;
    progress.script_state_flags[4] = 1;
    progress.script_state_flags[6] = 2;
    return progress;
}

bool writeFixture(
    const std::filesystem::path& save_path,
    const osf::WorldScene& world,
    const osf::PlayerData& player,
    const osf::RetailSaveProgress& progress,
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
        {false, 0, 0},
        world.playerGiantWarehouse(),
        world.playerAutomaticItems(),
        0x44,
        &error);
}

bool openMalseConversation(osf::WorldScene& world) {
    return osf::test::openNpcConversation(world, 1);
}

bool openSyriaConversation(osf::WorldScene& world) {
    return osf::test::openNpcConversation(world, 2);
}

bool testPostDustyRuinsTownRewards(
    const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest new_player;
    new_player.name = "TownRewards";
    std::string error;
    if (!check(
            seed.loadInitialScenario(
                data_root, new_player, &error),
            "The post-Dusty Ruins fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::PlayerData level_thirty = seed.playerData();
    if (!check(
            raiseToLevel(
                level_thirty, 30, seed.parameterTables()),
            "The town-reward fixture could not reach its authored level.")) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_episode_one_town_rewards_test";
    const std::filesystem::path ready_save =
        fixture_root / "ready" / "Save" / "0000.Ssv";
    const std::filesystem::path rewarded_save =
        fixture_root / "rewarded" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);

    if (!check(
            writeFixture(
                ready_save,
                seed,
                level_thirty,
                postDustyRuinsProgress(seed),
                error),
            "The post-Dusty Ruins town fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }

    osf::WorldScene rewarded;
    if (!check(
            loadSavedFixture(
                data_root, ready_save, rewarded, error) &&
                openMalseConversation(rewarded) &&
                rewarded.conversationMessageId() == 1000025 &&
                rewarded.retailSaveProgress()
                        .script_state_flags[8] == 1 &&
                rewarded.groundItems().empty(),
            "Malse did not begin his authored post-recovery callback.")) {
        std::cerr << error << '\n';
        return false;
    }
    rewarded.advanceConversation();
    if (!check(
            rewarded.conversationMessageId() == 1000026 &&
                rewarded.groundItems().empty(),
            "Malse skipped the Cold Svalt introduction or rewarded early.")) {
        return false;
    }
    rewarded.advanceConversation();
    if (!check(
            rewarded.conversationMessageId() == 1000027 &&
                rewarded.groundItems().size() == 1 &&
                rewarded.groundItems().front().item.category == 2 &&
                rewarded.groundItems().front().item.definition_id ==
                    1100000,
            "Malse's third callback did not create his one-time gift.")) {
        return false;
    }
    rewarded.takeAudioSamples();
    for (std::int32_t update = 0; update < 19; ++update) {
        rewarded.update();
    }
    const std::vector<std::int32_t> malse_audio =
        rewarded.takeAudioSamples();
    if (!check(
            std::count(
                malse_audio.begin(), malse_audio.end(), 93) == 1,
            "Malse's gift did not play its retail landing sound.")) {
        return false;
    }
    rewarded.advanceConversation();
    if (!check(
            !rewarded.conversationActive(),
            "Malse did not release the post-recovery conversation.")) {
        return false;
    }

    if (!check(
            openSyriaConversation(rewarded) &&
                rewarded.conversationMessageId() == 1000042 &&
                rewarded.retailSaveProgress()
                        .script_state_flags[7] == 1 &&
                rewarded.groundItems().size() == 1,
            "Syria did not begin her authored post-recovery callback.")) {
        return false;
    }
    rewarded.advanceConversation();
    if (!check(
            rewarded.conversationMessageId() == 1000043 &&
                rewarded.groundItems().size() == 2 &&
                rewarded.groundItems().back().item.category == 2 &&
                rewarded.groundItems().back().item.definition_id ==
                    1100002,
            "Syria's second message did not create her one-time gift.")) {
        return false;
    }
    rewarded.takeAudioSamples();
    for (std::int32_t update = 0; update < 19; ++update) {
        rewarded.update();
    }
    const std::vector<std::int32_t> syria_audio =
        rewarded.takeAudioSamples();
    if (!check(
            std::count(
                syria_audio.begin(), syria_audio.end(), 93) == 1,
            "Syria's gift did not play its retail landing sound.")) {
        return false;
    }
    rewarded.advanceConversation();
    if (!check(
            !rewarded.conversationActive(),
            "Syria did not release the post-recovery conversation.")) {
        return false;
    }

    if (!check(
            writeFixture(
                rewarded_save,
                rewarded,
                rewarded.playerData(),
                rewarded.retailSaveProgress(),
                error),
            "The post-recovery reward latches could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene persisted;
    if (!check(
            loadSavedFixture(
                data_root, rewarded_save, persisted, error) &&
                persisted.retailSaveProgress()
                        .script_state_flags[7] == 1 &&
                persisted.retailSaveProgress()
                        .script_state_flags[8] == 1 &&
                openMalseConversation(persisted) &&
                persisted.conversationMessageId() == 1000013,
            "Malse's saved reward latch repeated his gift.")) {
        std::cerr << error << '\n';
        return false;
    }
    persisted.advanceConversation();
    if (!check(
            openSyriaConversation(persisted) &&
                persisted.conversationMessageId() == 1000038 &&
                persisted.groundItems().empty(),
            "Syria's saved reward latch repeated her gift.")) {
        return false;
    }
    persisted.advanceConversation();

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
            data_root / "Scenario" / "00000000")) {
        return 0;
    }
    return testPostDustyRuinsTownRewards(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
