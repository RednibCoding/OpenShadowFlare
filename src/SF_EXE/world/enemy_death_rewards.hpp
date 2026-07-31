#ifndef OPENSHADOWFLARE_ENEMY_DEATH_REWARDS_HPP
#define OPENSHADOWFLARE_ENEMY_DEATH_REWARDS_HPP

#include "enemy_damage_receiver.hpp"
#include "items/player_inventory.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace osf {

class ItemDatabase;
class PlayerData;
class PlayerEquipment;
class RetailRandom;
class TableDatabase;

struct EnemyDeathDrop {
    InventoryItem item;
    WorldPosition position;
};

struct EnemyKillAccountingResult {
    std::int32_t experience_awarded = 0;
    bool direct_local_kill = false;
    bool level_gained = false;
    bool companion_level_gained = false;
    std::string level_up_notice;
    std::int32_t level_up_notice_counter = 0;
    std::vector<std::int32_t> audio_samples;
};

EnemyKillAccountingResult accountRetailEnemyKill(
    PlayerData& player,
    const EnemyDamageReceiverState& enemy,
    std::int32_t experience_reward,
    std::int32_t local_player_slot,
    std::int32_t main_hand_subtype,
    const TableDatabase& tables,
    bool companion_alive = false);

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
    RetailRandom& random);

}  // namespace osf

#endif
