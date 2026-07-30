#include "enemy_death_rewards.hpp"

#include "core/retail_random.hpp"
#include "items/item_database.hpp"
#include "items/item_instance_factory.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "player_data.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace osf {
namespace {

constexpr std::int32_t kGoldCategory = 4;
constexpr std::int32_t kGoldDefinition = 0;
constexpr double kTwoPi = 6.28318530717958647692;

std::int32_t nineDigitRandom(RetailRandom& random) {
    std::int32_t result = 0;
    for (std::int32_t digit = 0; digit < 9; ++digit) {
        result = result * 10 + random.next() % 10;
    }
    return result;
}

bool itemMatchesLootProfile(
    const ItemDefinition& item,
    std::int32_t category,
    std::int32_t lower_level,
    std::int32_t upper_level,
    std::int32_t episode_mask,
    const std::array<bool, 4>& variants) {
    if (item.variant < 0 || item.variant >= 4 ||
        !variants[static_cast<std::size_t>(item.variant)] ||
        (item.loot_episode_mask & episode_mask) == 0) {
        return false;
    }
    if (category == 4) {
        return true;
    }
    return
        (upper_level == -1 ||
         item.required_level <= upper_level) &&
        (lower_level == -1 ||
         item.required_level >= lower_level);
}

const ItemDefinition* chooseWeightedItem(
    std::int32_t category,
    std::int32_t lower_level,
    std::int32_t upper_level,
    std::int32_t episode_mask,
    const std::array<bool, 4>& variants,
    const ItemDatabase& items,
    RetailRandom& random) {
    std::int64_t total_weight = 0;
    const std::size_t first_category =
        category < 0 ? 0u : static_cast<std::size_t>(category);
    const std::size_t last_category =
        category < 0 ? 2u : static_cast<std::size_t>(category);
    if (first_category >= ItemDatabase::category_count ||
        last_category >= ItemDatabase::category_count) {
        return nullptr;
    }
    for (std::size_t item_category = first_category;
         item_category <= last_category;
         ++item_category) {
        for (const ItemDefinition& item :
             items.definitions(item_category)) {
            if (itemMatchesLootProfile(
                    item,
                    static_cast<std::int32_t>(item_category),
                    lower_level,
                    upper_level,
                    episode_mask,
                    variants) &&
                item.loot_weight > 0) {
                total_weight += item.loot_weight;
            }
        }
    }
    if (total_weight <= 0 ||
        total_weight >
            std::numeric_limits<std::int32_t>::max()) {
        return nullptr;
    }

    const std::int32_t offset =
        nineDigitRandom(random) %
        static_cast<std::int32_t>(total_weight);
    std::int64_t cumulative = 0;
    for (std::size_t item_category = first_category;
         item_category <= last_category;
         ++item_category) {
        for (const ItemDefinition& item :
             items.definitions(item_category)) {
            if (!itemMatchesLootProfile(
                    item,
                    static_cast<std::int32_t>(item_category),
                    lower_level,
                    upper_level,
                    episode_mask,
                    variants) ||
                item.loot_weight <= 0) {
                continue;
            }
            cumulative += item.loot_weight;
            if (offset < cumulative) {
                return &item;
            }
        }
    }
    return nullptr;
}

}  // namespace

EnemyKillAccountingResult accountRetailEnemyKill(
    PlayerData& player,
    const EnemyDamageReceiverState& enemy,
    std::int32_t experience_reward,
    std::int32_t local_player_slot,
    std::int32_t main_hand_subtype,
    const TableDatabase& tables) {
    EnemyKillAccountingResult result;
    const TableData* shares = tables.find(14);
    if (player.level() < 100 &&
        local_player_slot >= 0 &&
        static_cast<std::size_t>(local_player_slot) <
            enemy.attributed_damage.size() &&
        enemy.maximum_life > 0 &&
        enemy.attributed_damage[
            static_cast<std::size_t>(local_player_slot)] != 0 &&
        shares) {
        const std::int32_t bucket = std::clamp(
            enemy.attributed_damage[
                static_cast<std::size_t>(local_player_slot)] *
                10 /
                enemy.maximum_life,
            0,
            10);
        result.experience_awarded =
            shares->value(bucket, 0) *
            experience_reward /
            100;
        player.addExperience(result.experience_awarded);
    }

    if (enemy.defeat_source_character_number ==
        local_player_slot) {
        result.direct_local_kill = true;
        const std::size_t kind =
            !enemy.defeated_by_effect &&
                    main_hand_subtype >= 0 &&
                    main_hand_subtype < 8 &&
                    main_hand_subtype != 8 &&
                    main_hand_subtype != 9
                ? static_cast<std::size_t>(
                      main_hand_subtype)
                : 8u;
        player.addKillCount(kind);
    }
    result.level_gained =
        player.applyLevelThreshold(tables);
    return result;
}

std::vector<EnemyDeathDrop> createRetailEnemyDrops(
    std::int32_t loot_table_row,
    std::int32_t gold_drop_chance,
    std::int32_t gold_minimum,
    std::int32_t gold_maximum,
    WorldPosition position,
    const ObjectBounds& judgement,
    std::int32_t gold_find_bonus,
    std::int32_t episode_mask,
    std::int32_t active_player_count,
    const TableDatabase& tables,
    const ItemDatabase& items,
    RetailRandom& random) {
    std::vector<EnemyDeathDrop> drops;
    const TableData* loot = tables.find(30);
    const TableData* profiles = tables.find(31);
    if (loot && profiles &&
        loot_table_row >= 0 &&
        loot->contains(loot_table_row, 21)) {
        std::int32_t attempts =
            loot->value(loot_table_row, 0);
        if (attempts == 0) {
            attempts = active_player_count;
        }
        attempts = std::max(attempts, 0);
        std::array<bool, 4> variants{};
        std::int32_t successful = 0;
        for (std::int32_t attempt = 0;
             attempt < attempts;
             ++attempt) {
            if (random.next() % 100 >
                loot->value(loot_table_row, 1)) {
                continue;
            }
            const std::int32_t slot =
                random.next() % 10;
            const std::int32_t profile_row =
                loot->value(
                    loot_table_row, slot * 2 + 2);
            const std::int32_t encoded_variants =
                loot->value(
                    loot_table_row, slot * 2 + 3);
            if (!profiles->contains(profile_row, 4)) {
                continue;
            }
            variants[0] =
                profiles->value(profile_row, 0) != 0 ||
                encoded_variants % 10 != 0;
            if ((encoded_variants / 10) % 10 != 0) {
                variants[1] = true;
            }
            if ((encoded_variants / 100) % 10 != 0) {
                variants[2] = true;
            }
            variants[3] = true;

            const ItemDefinition* definition =
                chooseWeightedItem(
                    profiles->value(profile_row, 3),
                    profiles->value(profile_row, 2),
                    profiles->value(profile_row, 1),
                    profiles->value(profile_row, 4) &
                        episode_mask,
                    variants,
                    items,
                    random);
            if (!definition) {
                continue;
            }
            const double angle =
                attempts > 0
                    ? static_cast<double>(successful) *
                          kTwoPi /
                          static_cast<double>(attempts)
                    : 0.0;
            drops.push_back({
                makeRetailInventoryItem(
                    *definition,
                    [&random]() {
                        return random.next();
                    }),
                {
                    position.x +
                        static_cast<std::int32_t>(
                            std::cos(angle) * 200.0),
                    position.y -
                        static_cast<std::int32_t>(
                            std::sin(angle) * 200.0),
                },
            });
            ++successful;
        }
    }

    const std::int32_t multiplier =
        100 + gold_find_bonus;
    if (multiplier > 0 &&
        gold_minimum >= 0 &&
        gold_maximum >= gold_minimum &&
        random.next() % 100 < gold_drop_chance) {
        const ItemDefinition* gold =
            items.find(kGoldCategory, kGoldDefinition);
        const std::int64_t range =
            static_cast<std::int64_t>(gold_maximum) -
            gold_minimum + 1;
        if (gold && range > 0 &&
            range <=
                std::numeric_limits<std::int32_t>::max()) {
            std::int32_t quantity =
                (random.next() %
                     static_cast<std::int32_t>(range) +
                 gold_minimum) *
                multiplier /
                100;
            quantity = std::max(quantity, 1);
            drops.push_back({
                makeRetailInventoryItem(
                    *gold,
                    [&random]() {
                        return random.next();
                    },
                    quantity),
                {
                    position.x + judgement.left,
                    position.y + judgement.top + 100,
                },
            });
        }
    }
    return drops;
}

}  // namespace osf
