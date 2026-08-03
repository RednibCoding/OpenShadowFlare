#include "core/retail_random.hpp"
#include "episode_one_test_support.hpp"
#include "items/player_automatic_items.hpp"
#include "items/player_inventory.hpp"
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

bool openMalseConversation(
    osf::WorldScene& world,
    std::vector<std::int32_t>* audio = nullptr) {
    return osf::test::openNpcConversation(world, 1, audio);
}

osf::RetailSaveProgress malseQuestProgress(
    const osf::WorldScene& seed,
    std::int32_t stolen_gem_state) {
    osf::RetailSaveProgress progress =
        seed.retailSaveProgress();
    if (progress.quest_flags.size() < 48) {
        progress.quest_flags.resize(48, 0);
    }
    progress.quest_flags[0] = 2;
    progress.quest_flags[1] = stolen_gem_state;
    if (progress.script_state_flags.size() < 72) {
        progress.script_state_flags.resize(72, 0);
    }
    // Malse's opening visit sets flag 9. His post-Red-Goblin introduction
    // advances flag 6 from zero to two before offering the gem quest.
    progress.script_state_flags[9] = 1;
    if (stolen_gem_state != 0) {
        progress.script_state_flags[6] = 2;
    }
    return progress;
}

bool writeQuestFixture(
    const std::filesystem::path& save_path,
    const osf::WorldScene& seed,
    const osf::RetailSaveProgress& progress,
    const osf::PlayerAutomaticItems& automatic_items,
    std::string& error) {
    return osf::writeRetailSave(
        save_path,
        seed.playerData(),
        seed.itemDatabase(),
        seed.playerInventory(),
        seed.playerEquipment(),
        seed.playerBelt(),
        seed.playerSpecialItems(),
        progress,
        seed.playerMagic(),
        seed.playerMineCount(),
        {false, 0, 0},
        seed.playerGiantWarehouse(),
        automatic_items,
        0x35,
        &error);
}

bool testBlackHammerDrop(
    const std::filesystem::path& data_root,
    const osf::WorldScene& seed) {
    osf::ScenarioData west_ruins;
    std::string error;
    if (!check(
            west_ruins.load(
                data_root / "Scenario" / "00000004" /
                    "Scenario.Mct",
                &error),
            "The West Ruins scenario could not be decoded.")) {
        std::cerr << error << '\n';
        return false;
    }
    const auto black_hammer = std::find_if(
        west_ruins.enemies().begin(),
        west_ruins.enemies().end(),
        [](const osf::ScenarioEnemy& enemy) {
            return enemy.id == 0 &&
                   enemy.name == "Black Hammer";
        });
    if (!check(
            black_hammer != west_ruins.enemies().end() &&
                black_hammer->loot_table_row == 6,
            "The authored Black Hammer loot owner differs from retail.")) {
        return false;
    }

    osf::RetailRandom random(1);
    const std::vector<osf::EnemyDeathDrop> drops =
        osf::createRetailEnemyDrops(
            black_hammer->loot_table_row,
            black_hammer->gold_drop_chance,
            black_hammer->gold_minimum,
            black_hammer->gold_maximum,
            {1000, 2000},
            {
                black_hammer->judgement_left,
                black_hammer->judgement_top,
                black_hammer->judgement_right,
                black_hammer->judgement_bottom,
            },
            0,
            1,
            1,
            seed.parameterTables(),
            seed.itemDatabase(),
            random);
    const osf::ItemDefinition* gem =
        seed.itemDatabase().find(4, 99000000);
    return check(
        gem && gem->automatic_inventory_page == 0 &&
            drops.size() == 1 &&
            drops.front().item.category == 4 &&
            drops.front().item.definition_id == 99000000 &&
            drops.front().position.x == 1200 &&
            drops.front().position.y == 2000,
        "Black Hammer did not produce the fixed automatic-owner gem drop.");
}

bool testMalseQuestFlow(const std::filesystem::path& data_root) {
    osf::WorldScene seed;
    osf::PlayerLoadRequest player;
    player.name = "GemQuest";
    std::string error;
    if (!check(
            seed.loadInitialScenario(data_root, player, &error),
            "The stolen-gem save fixture could not load Remote Town.")) {
        std::cerr << error << '\n';
        return false;
    }
    if (!testBlackHammerDrop(data_root, seed)) {
        return false;
    }

    const std::filesystem::path fixture_root =
        std::filesystem::temp_directory_path() /
        "openshadowflare_stolen_gem_quest_test";
    const std::filesystem::path offer_save =
        fixture_root / "offer" / "Save" / "0000.Ssv";
    const std::filesystem::path return_save =
        fixture_root / "return" / "Save" / "0000.Ssv";
    const std::filesystem::path completed_save =
        fixture_root / "completed" / "Save" / "0000.Ssv";
    std::error_code cleanup_error;
    std::filesystem::remove_all(fixture_root, cleanup_error);

    osf::PlayerAutomaticItems no_automatic_items;
    if (!check(
            writeQuestFixture(
                offer_save,
                seed,
                malseQuestProgress(seed, 0),
                no_automatic_items,
                error),
            "The stolen-gem offer fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene offered;
    const bool offer_loaded = loadSavedFixture(
        data_root, offer_save, offered, error);
    const bool offer_opened =
        offer_loaded && openMalseConversation(offered);
    if (!check(
            offer_loaded && offer_opened &&
                offered.conversationMessageId() == 1000021,
            "Malse did not begin his post-Red-Goblin introduction.")) {
        std::cerr << error << " loaded=" << offer_loaded
                  << " opened=" << offer_opened
                  << " message="
                  << offered.conversationMessageId()
                  << " q0=" << offered.quests().state(0)
                  << " q1=" << offered.quests().state(1)
                  << " npcs=" << offered.npcs().size()
                  << " player=" << offered.playerWorldX()
                  << ',' << offered.playerWorldY()
                  << " camera=" << offered.cameraScreenX()
                  << ',' << offered.cameraScreenY()
                  << '\n';
        for (const osf::NpcActor& npc : offered.npcs()) {
            std::cerr << "npc " << npc.id() << ' '
                      << npc.name() << " pos="
                      << npc.position().x << ','
                      << npc.position().y << " visible="
                      << npc.visible() << " pointer="
                      << npc.pointerEnabled() << '\n';
        }
        return false;
    }
    offered.advanceConversation();
    if (!check(
            offered.conversationMessageId() == 1000022,
            "Malse's introduction skipped its second message.")) {
        return false;
    }
    offered.advanceConversation();
    if (!check(
            offered.conversationMessageId() == 1000023,
            "Malse's introduction skipped his merchant name.")) {
        return false;
    }
    offered.advanceConversation();
    const std::vector<std::int32_t> offer_audio =
        offered.takeAudioSamples();
    const osf::RetailSaveProgress offered_progress =
        offered.retailSaveProgress();
    if (!check(
            offered.conversationMessageId() == 1000024 &&
                offered.quests().state(1) == 1 &&
                offered.quests().lastCue() ==
                    osf::QuestCue::updated &&
                offered.quests().notice().quest_id == 1 &&
                offered.quests().notice().counter == 600 &&
                containsSample(offer_audio, 65) &&
                offered_progress.script_state_flags.size() > 6 &&
                offered_progress.script_state_flags[6] == 2,
            "Malse did not offer the gem quest through its authored state, "
            "notice, and cue.")) {
        return false;
    }
    offered.advanceConversation();
    if (!check(
            !offered.conversationActive(),
            "The stolen-gem offer did not release Malse.")) {
        return false;
    }

    const osf::ItemDefinition* gem =
        seed.itemDatabase().find(4, 99000000);
    osf::PlayerAutomaticItems returned_items;
    if (!check(
            gem && returned_items.add(
                       *gem, osf::makeInventoryItem(*gem)),
            "The authored stolen gem could not enter its automatic page.")) {
        return false;
    }
    if (!check(
            writeQuestFixture(
                return_save,
                seed,
                malseQuestProgress(seed, 1),
                returned_items,
                error),
            "The stolen-gem return fixture could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene returned;
    std::vector<std::int32_t> completion_audio;
    if (!check(
            loadSavedFixture(
                data_root, return_save, returned, error) &&
                returned.playerAutomaticItems().contains(
                    4, 99000000) &&
                openMalseConversation(
                    returned, &completion_audio) &&
                returned.conversationMessageId() == 1000028 &&
                returned.quests().state(1) == 2 &&
                returned.quests().lastCue() ==
                    osf::QuestCue::completed &&
                !returned.playerAutomaticItems().contains(
                    4, 99000000) &&
                containsSample(completion_audio, 66),
            "Malse did not recognize the returned stolen gem.")) {
        std::cerr << error << '\n';
        return false;
    }
    returned.advanceConversation();
    if (!check(
            returned.conversationMessageId() == 1000029 &&
                returned.quests().state(1) == 2 &&
                returned.quests().lastCue() ==
                    osf::QuestCue::completed &&
                !returned.playerAutomaticItems().contains(
                    4, 99000000),
            "Returning the stolen gem did not remove it and complete the "
            "quest with the retail cue.")) {
        std::cerr << "message=" << returned.conversationMessageId()
                  << " state=" << returned.quests().state(1)
                  << " cue="
                  << static_cast<std::int32_t>(
                         returned.quests().lastCue())
                  << " gem="
                  << returned.playerAutomaticItems().contains(
                         4, 99000000)
                  << " audio=";
        for (std::int32_t sample : completion_audio) {
            std::cerr << sample << ',';
        }
        std::cerr << '\n';
        return false;
    }
    returned.advanceConversation();
    if (!check(
            returned.conversationMessageId() == 1000030,
            "The stolen-gem completion skipped Malse's thanks.")) {
        return false;
    }
    returned.advanceConversation();
    if (!check(
            returned.conversationMessageId() == 1000031,
            "The stolen-gem completion skipped Malse's information.")) {
        return false;
    }
    returned.advanceConversation();
    if (!check(
            !returned.conversationActive() &&
                returned.quests().state(1) == 2 &&
                !returned.playerAutomaticItems().contains(
                    4, 99000000),
            "Malse retained the gem or lost the completed quest after "
            "release.")) {
        return false;
    }
    if (!check(
            osf::writeRetailSave(
                completed_save,
                returned.playerData(),
                returned.itemDatabase(),
                returned.playerInventory(),
                returned.playerEquipment(),
                returned.playerBelt(),
                returned.playerSpecialItems(),
                returned.retailSaveProgress(),
                returned.playerMagic(),
                returned.playerMineCount(),
                returned.retailSaveWorldState(),
                returned.playerGiantWarehouse(),
                returned.playerAutomaticItems(),
                0x36,
                &error),
            "The completed stolen-gem quest could not be saved.")) {
        std::cerr << error << '\n';
        return false;
    }
    osf::WorldScene persisted;
    const bool completed =
        loadSavedFixture(
            data_root, completed_save, persisted, error) &&
        persisted.quests().state(1) == 2 &&
        !persisted.playerAutomaticItems().contains(
            4, 99000000) &&
        openMalseConversation(persisted) &&
        persisted.conversationMessageId() == 1000013 &&
        persisted.conversationRequiresSelection() &&
        !containsSample(persisted.takeAudioSamples(), 66);
    if (persisted.conversationActive()) {
        persisted.chooseConversationOption(3);
    }
    std::filesystem::remove_all(fixture_root, cleanup_error);
    return check(
        completed,
        "Saving and loading repeated the stolen-gem completion or restored "
        "the removed quest item.");
}

}  // namespace

int main() {
#ifdef OPENSHADOWFLARE_SOURCE_DIR
    const std::filesystem::path data_root =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    if (!std::filesystem::is_directory(
            data_root / "Scenario" / "00000004")) {
        return 0;
    }
    return testMalseQuestFlow(data_root) ? 0 : 1;
#else
    return 0;
#endif
}
