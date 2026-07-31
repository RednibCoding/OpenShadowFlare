#include "world_scene.hpp"

#include "core/retail_integer.hpp"

namespace osf {
namespace {

constexpr std::int32_t kSpecialItemCategory = 4;
// FUN_0044f2f0 adds five percentage-rate points for these two authored
// special items after summing equipped instance parameters 17 and 18.
constexpr std::int32_t kLifeRecoveryItem = 98000003;
constexpr std::int32_t kManaRecoveryItem = 98000004;

}  // namespace

void WorldScene::updatePlayerResourceRates() {
    const PlayerRuntimeProfile profile =
        playerRuntimeProfile();
    const std::int32_t life_rate = retailAdd(
        player_equipment_.instanceParameterBonus(
            17, item_database_),
        player_special_items_.contains(
            kSpecialItemCategory, kLifeRecoveryItem)
            ? 5
            : 0);
    const PlayerResourceRateUpdate life_update =
        player_life_rate_.update(
            player_data_.currentLife(),
            profile.maximum_life,
            life_rate,
            1,
            player_data_.currentLife() > 0);
    if (life_update.changed) {
        player_data_.setCurrentLife(
            life_update.value, profile.maximum_life);
    }

    const std::int32_t mana_rate = retailAdd(
        retailAdd(
            player_equipment_.instanceParameterBonus(
                18, item_database_),
            player_special_items_.contains(
                kSpecialItemCategory, kManaRecoveryItem)
                ? 5
                : 0),
        retailAdd(
            player_moon_spell_.manaChangeRate(),
            player_berserker_spell_.manaChangeRate()));
    const PlayerResourceRateUpdate mana_update =
        player_mana_rate_.update(
            player_data_.currentMana(),
            profile.maximum_mana,
            mana_rate,
            0);
    if (mana_update.changed) {
        player_data_.setCurrentMana(
            mana_update.value, profile.maximum_mana);
    }

    const PlayerSustainedSpellShutdown shutdown =
        deactivateSustainedSpellsAtZeroMana(
            player_data_.currentMana(),
            player_moon_spell_,
            player_berserker_spell_);
    if (shutdown.moon_deactivated) {
        refreshCompanionRuntimeProfile();
    }
    if (shutdown.berserker_deactivated) {
        refreshPlayerRuntimeProfile();
    }
    if (player_data_.currentMana() == 0) {
        player_energy_shield_.deactivate();
    }
}

}  // namespace osf
