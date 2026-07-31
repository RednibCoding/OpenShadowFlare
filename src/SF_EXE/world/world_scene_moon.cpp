#include "world_scene.hpp"

namespace osf {

void WorldScene::refreshCompanionRuntimeProfile(
    bool level_gained) {
    if (!hasCompanion()) {
        return;
    }
    CompanionProfile base;
    if (!decodeCompanionProfile(
            parameter_tables_,
            player_data_.companionType(),
            player_data_.companionLevel(),
            base)) {
        return;
    }
    const CompanionProfile profile =
        applyPlayerMoonCompanionModifiers(
            base, player_moon_spell_, parameter_tables_);
    if (level_gained) {
        companion_.applyLevelProfile(profile);
    } else {
        companion_.applyRuntimeProfile(profile);
    }
}

void WorldScene::updatePlayerMoonSpell() {
    const PlayerMoonManaUpdate update =
        player_moon_spell_.updateMana(
            player_data_.currentMana(),
            player_data_.baseMaximumMana());
    if (update.changed) {
        player_data_.setCurrentMana(update.mana);
    }
    if (update.deactivated) {
        refreshCompanionRuntimeProfile();
    }
}

}  // namespace osf
