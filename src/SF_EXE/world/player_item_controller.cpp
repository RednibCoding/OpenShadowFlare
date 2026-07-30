#include "player_item_controller.hpp"

#include "items/item_database.hpp"
#include "items/player_belt.hpp"
#include "world/player_data.hpp"

namespace osf {

void PlayerItemController::clear() {
    mine_count_ = 0;
}

void PlayerItemController::initializeNew() {
    // FUN_00440f70 keeps mines outside the ordinary item containers.
    mine_count_ = 5;
}

BeltItemUseResult PlayerItemController::useBeltPocket(
    std::int32_t pocket,
    PlayerBelt& belt,
    const ItemDatabase& item_database,
    PlayerData& player) {
    if (pocket < 0 || pocket >= 8) {
        return {};
    }
    const std::int32_t grid_x = pocket % 4;
    const std::int32_t grid_y = pocket / 4;
    const InventoryItem* item =
        belt.itemAt(grid_x, grid_y);
    if (!item || item->category != 3) {
        return {};
    }
    const ItemDefinition* definition =
        item_database.find(
            item->category,
            item->definition_id);
    if (!definition) {
        return {};
    }

    // FUN_0044a240 consumes medicine only when at least one represented
    // effect changes its target. Companion and timed-status owners will
    // join this path when those systems are reconstructed.
    if (definition->restore_companion_life != 0 ||
        definition->restore_companion_life_percent != 0 ||
        definition->consumable_effect != -1) {
        return {};
    }
    const bool life_changed =
        player.restoreLife(
            definition->restore_life,
            definition->restore_life_percent);
    const bool mana_changed =
        player.restoreMana(
            definition->restore_mana,
            definition->restore_mana_percent);
    const bool changed =
        life_changed || mana_changed;
    if (!changed ||
        !belt.takeAt(grid_x, grid_y)) {
        return {};
    }
    return {true, 16};
}

std::int32_t PlayerItemController::mineCount() const {
    return mine_count_;
}

}  // namespace osf
