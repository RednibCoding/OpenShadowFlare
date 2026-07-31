#include "world_scene.hpp"

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
}

}  // namespace osf
