#include "core/retail_random.hpp"
#include "items/item_database.hpp"
#include "items/item_instance_factory.hpp"
#include "items/player_inventory.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "world/enemy_death_rewards.hpp"
#include "world/ground_item.hpp"
#include "world/player_data.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

}  // namespace

int main() {
    const std::filesystem::path data_root =
        std::filesystem::path(
            OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare" / "System" / "Game" /
        "Parameter";
    if (!std::filesystem::is_directory(data_root)) {
        return 0;
    }

    osf::ItemDatabase items;
    osf::TableDatabase tables;
    std::string error;
    if (!check(
            items.load(data_root / "Item.Ibn", &error) &&
                tables.load(data_root / "Table.Tbd", &error),
            "Retail reward data could not be loaded.")) {
        std::cerr << error << '\n';
        return 1;
    }

    const osf::ItemDefinition* short_sword =
        items.find(0, 0);
    if (!check(
            short_sword &&
                short_sword->loot_episode_mask == 1 &&
                short_sword->loot_weight == 15000 &&
                short_sword
                        ->instance_parameter_rolls.size() ==
                    39 &&
                short_sword->element_rolls.size() == 8,
            "The retail loot and roll fields were decoded from the "
            "wrong Item.Ibn offsets.")) {
        return 1;
    }

    osf::RetailRandom first_roll(47);
    osf::RetailRandom second_roll(47);
    const osf::InventoryItem rolled =
        osf::makeRetailInventoryItem(
            *short_sword,
            [&first_roll]() {
                return first_roll.next();
            });
    const osf::InventoryItem repeated =
        osf::makeRetailInventoryItem(
            *short_sword,
            [&second_roll]() {
                return second_roll.next();
            });
    if (!check(
            rolled.retail_state.size() == 200 &&
                rolled.retail_state ==
                    repeated.retail_state &&
                first_roll.state() ==
                    second_roll.state(),
            "Retail item construction is not deterministic or uses "
            "the wrong instance-state size.")) {
        return 1;
    }

    std::vector<osf::GroundItem> ground;
    osf::PlayerInventory inventory;
    if (!check(
            osf::createGroundItem(
                ground, rolled, {100, 200}) &&
                inventory.store(ground.front().item) &&
                inventory.items().size() == 1 &&
                inventory.items().front().retail_state ==
                    rolled.retail_state &&
                inventory.items().front().durability ==
                    rolled.durability &&
                inventory.items().front().identified ==
                    rolled.identified,
            "Ground pickup discarded rolled item state.")) {
        return 1;
    }

    osf::PlayerData player;
    if (!check(
            player.initializeNew(
                "Mina", 0, tables, &error),
            "The reward test player could not be initialized.")) {
        return 1;
    }
    const std::int32_t old_life =
        player.baseMaximumLife();
    osf::EnemyDamageReceiverState defeated;
    defeated.maximum_life = 100;
    defeated.attributed_damage[0] = 100;
    defeated.defeat_source_character_number = 0;
    const osf::EnemyKillAccountingResult accounting =
        osf::accountRetailEnemyKill(
            player,
            defeated,
            25,
            0,
            0,
            tables);
    if (!check(
                accounting.experience_awarded == 25 &&
                accounting.direct_local_kill &&
                accounting.level_gained &&
                accounting.level_up_notice_counter == 900 &&
                accounting.level_up_notice.rfind(
                    "Level 2\n", 0) == 0 &&
                !accounting.level_up_notice.empty() &&
                accounting.audio_samples ==
                    std::vector<std::int32_t>{63} &&
                player.level() == 2 &&
                player.experience() == 0 &&
                player.totalKillCount() == 1 &&
                player.killCount(0) == 1 &&
                player.baseMaximumLife() > old_life &&
                player.currentLife() ==
                    player.baseMaximumLife(),
            "Kill attribution, experience, or novice level growth "
            "differs from the retail callback.")) {
        return 1;
    }
    osf::EnemyKillAccountingResult level_five;
    while (player.level() < 5) {
        level_five = osf::accountRetailEnemyKill(
            player,
            defeated,
            player.experienceThreshold(tables),
            0,
            0,
            tables);
    }
    if (!check(
            player.level() == 5 &&
                level_five.level_gained &&
                level_five.audio_samples ==
                    std::vector<std::int32_t>{64, 63},
            "The level-five occupation cue did not precede the "
            "ordinary retail level-up sound.")) {
        return 1;
    }

    osf::RetailRandom loot_random(19);
    const std::vector<osf::EnemyDeathDrop> loot =
        osf::createRetailEnemyDrops(
            0,
            0,
            0,
            0,
            {1000, 2000},
            {-20, -30, 20, 30},
            0,
            1,
            1,
            tables,
            items,
            loot_random);
    if (!check(
            loot.size() == 1 &&
                loot.front().item.category == 4 &&
                loot.front().item.definition_id == 1 &&
                items.find(4, 1) &&
                items.find(4, 1)->name == "Mine" &&
                items.find(4, 1)->required_level == 1 &&
                loot.front().position.x == 1200 &&
                loot.front().position.y == 2000 &&
                loot.front().item.retail_state.size() == 4,
            "The authored loot table no longer follows the retail "
            "chance, profile, weighting, and placement order.")) {
        return 1;
    }

    osf::RetailRandom gold_random(7);
    const std::vector<osf::EnemyDeathDrop> gold =
        osf::createRetailEnemyDrops(
            -1,
            100,
            10,
            10,
            {1000, 2000},
            {-20, -30, 20, 30},
            50,
            1,
            1,
            tables,
            items,
            gold_random);
    return check(
               gold.size() == 1 &&
                   gold.front().item.category == 4 &&
                   gold.front().item.definition_id == 0 &&
                   gold.front().item.quantity == 15 &&
                   gold.front().position.x == 980 &&
                   gold.front().position.y == 2070,
               "Gold chance, Gold Find, amount, or drop origin "
               "differs from the retail callback.")
        ? 0
        : 1;
}
