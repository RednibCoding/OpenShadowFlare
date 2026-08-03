#include "world_scene.hpp"

#include <algorithm>

namespace osf {

PlayerRuntimeProfile WorldScene::playerRuntimeProfile() const {
    return buildPlayerRuntimeProfile(
        player_data_,
        player_equipment_,
        item_database_,
        player_berserker_spell_,
        parameter_tables_);
}

void WorldScene::refreshPlayerRuntimeProfile() {
    const PlayerRuntimeProfile profile =
        playerRuntimeProfile();
    player_data_.setCurrentLife(
        player_data_.currentLife(), profile.maximum_life);
    player_data_.setCurrentMana(
        player_data_.currentMana(), profile.maximum_mana);
    player_.setWalkingSpeedTier(profile.walkingSpeedTier());
    player_item_controller_.restoreMineCount(
        std::min(
            player_item_controller_.mineCount(),
            playerMaximumMineCount()));
}

std::int32_t WorldScene::playerMaximumMineCount() const {
    // RebuildPlayerRuntimeProfile copies the base value at +0x160 and adds
    // equipped instance word 84 into the runtime value at +0x2c0.
    return std::max(
        10 + player_equipment_.instanceParameterBonus(
                 84, item_database_),
        std::int32_t{1});
}

std::int32_t WorldScene::playerMineDamageBonus() const {
    // Equipped instance word 81 feeds the runtime mine-effect value at
    // +0x2c4. The placement controller adds it to Table 23.
    return player_equipment_.instanceParameterBonus(
        81, item_database_);
}

}  // namespace osf
