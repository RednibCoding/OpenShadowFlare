#include "player_item_controller.hpp"

#include "items/item_database.hpp"
#include "items/player_belt.hpp"
#include "items/player_inventory.hpp"
#include "world/companion_actor.hpp"
#include "world/player_data.hpp"

#include <algorithm>

namespace osf {
namespace {

bool applyMedicine(
    const InventoryItem& item,
    const ItemDatabase& item_database,
    PlayerItemUseTargets targets) {
    if (item.category != 3) {
        return false;
    }
    const ItemDefinition* definition =
        item_database.find(
            item.category,
            item.definition_id);
    if (!definition) {
        return false;
    }

    const auto restored_value = [](
                                    std::int32_t current,
                                    std::int32_t maximum,
                                    std::int32_t amount,
                                    std::int32_t maximum_percent,
                                    std::int32_t bonus) {
        const std::int64_t scale =
            static_cast<std::int64_t>(bonus) + 100;
        const std::int64_t flat =
            static_cast<std::int64_t>(amount) * scale / 100;
        const std::int64_t percentage =
            (static_cast<std::int64_t>(maximum_percent) * maximum /
             100) *
            scale / 100;
        return static_cast<std::int32_t>(
            std::clamp<std::int64_t>(
                static_cast<std::int64_t>(current) +
                    flat + percentage,
                0,
                std::max<std::int32_t>(0, maximum)));
    };

    const std::int32_t old_life = targets.player.currentLife();
    const std::int32_t old_mana = targets.player.currentMana();
    targets.player.setCurrentLife(
        restored_value(
            old_life,
            targets.maximum_life,
            definition->restore_life,
            definition->restore_life_percent,
            targets.life_restoration_bonus),
        targets.maximum_life);
    targets.player.setCurrentMana(
        restored_value(
            old_mana,
            targets.maximum_mana,
            definition->restore_mana,
            definition->restore_mana_percent,
            targets.mana_restoration_bonus),
        targets.maximum_mana);
    if (targets.player.currentLife() != old_life ||
        targets.player.currentMana() != old_mana) {
        return true;
    }

    if (targets.companion &&
        targets.companion->restoreLife(
            definition->restore_companion_life,
            definition->restore_companion_life_percent)) {
        return true;
    }

    if (definition->consumable_effect == -2) {
        return targets.player.clearElementCondition();
    }
    if (definition->consumable_effect != -1) {
        return targets.player.applyElementMedicine(
            definition->consumable_effect,
            definition->consumable_effect_value);
    }
    return false;
}

}  // namespace

void PlayerItemController::clear() {
    mine_count_ = 0;
}

void PlayerItemController::initializeNew() {
    // FUN_00440f70 keeps mines outside the ordinary item containers.
    mine_count_ = 5;
}

void PlayerItemController::restoreMineCount(
    std::int32_t count) {
    mine_count_ = std::max<std::int32_t>(count, 0);
}

bool PlayerItemController::consumeMine() {
    if (mine_count_ <= 0) {
        return false;
    }
    --mine_count_;
    return true;
}

bool PlayerItemController::collectMine(
    std::int32_t maximum_count) {
    if (mine_count_ >= std::max<std::int32_t>(maximum_count, 0)) {
        return false;
    }
    ++mine_count_;
    return true;
}

PlayerItemUseResult PlayerItemController::useBeltPocket(
    std::int32_t pocket,
    PlayerBelt& belt,
    const ItemDatabase& item_database,
    PlayerItemUseTargets targets) {
    if (pocket < 0 || pocket >= 8) {
        return {};
    }
    const std::int32_t grid_x = pocket % 4;
    const std::int32_t grid_y = pocket / 4;
    const InventoryItem* item =
        belt.itemAt(grid_x, grid_y);
    if (!item ||
        !applyMedicine(
            *item,
            item_database,
            targets) ||
        !belt.takeAt(grid_x, grid_y)) {
        return {};
    }
    return {true, 16};
}

PlayerItemUseResult
PlayerItemController::useInventoryItem(
    std::int32_t item_index,
    PlayerInventory& inventory,
    const ItemDatabase& item_database,
    PlayerItemUseTargets targets) {
    if (item_index < 0 ||
        static_cast<std::size_t>(item_index) >=
            inventory.items().size()) {
        return {};
    }
    const InventoryItem& item =
        inventory.items()[
            static_cast<std::size_t>(item_index)];
    if (!applyMedicine(
            item,
            item_database,
            targets) ||
        !inventory.take(
            static_cast<std::size_t>(item_index))) {
        return {};
    }
    return {true, 16};
}

std::int32_t PlayerItemController::mineCount() const {
    return mine_count_;
}

}  // namespace osf
