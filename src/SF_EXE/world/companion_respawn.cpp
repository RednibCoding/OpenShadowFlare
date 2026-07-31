#include "companion_respawn.hpp"

#include "items/player_inventory.hpp"

#include <algorithm>

namespace osf {
namespace {

constexpr std::int32_t kNormalRespawnUpdates = 900;
constexpr std::int32_t kQuickRespawnUpdates = 600;
constexpr std::int32_t kQuickRespawnItem = 98000002;

}  // namespace

std::int32_t retailCompanionRespawnUpdates(
    const PlayerInventory& inventory) {
    const bool quick_respawn =
        std::any_of(
            inventory.items().begin(),
            inventory.items().end(),
            [](const InventoryItem& item) {
                return item.category == 4 &&
                       item.definition_id ==
                           kQuickRespawnItem;
            });
    return quick_respawn
        ? kQuickRespawnUpdates
        : kNormalRespawnUpdates;
}

}  // namespace osf
