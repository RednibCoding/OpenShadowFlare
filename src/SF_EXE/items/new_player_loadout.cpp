#include "new_player_loadout.hpp"

#include "item_database.hpp"
#include "player_belt.hpp"
#include "player_equipment.hpp"
#include "player_inventory.hpp"

#include <utility>

namespace osf {
namespace {

void setError(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

}  // namespace

bool initializeRetailNewPlayerLoadout(
    const ItemDatabase& item_database,
    PlayerInventory& inventory,
    PlayerEquipment& equipment,
    PlayerBelt& belt,
    std::int32_t player_level,
    std::string* error) {
    const ItemDefinition* leather_cloth =
        item_database.find(1, 0);
    const ItemDefinition* tablet =
        item_database.find(3, 0);
    const ItemDefinition* capsule =
        item_database.find(3, 10000000);
    if (!leather_cloth || !tablet || !capsule) {
        setError(
            error,
            "The retail new-character items are missing.");
        return false;
    }

    PlayerInventory new_inventory;
    PlayerEquipment new_equipment;
    PlayerBelt new_belt;
    if (!new_equipment
             .place(
                 EquipmentSlot::body,
                 makeInventoryItem(*leather_cloth),
                 *leather_cloth,
                 player_level)
             .accepted) {
        setError(
            error,
            "The initial Leather Cloth could not be equipped.");
        return false;
    }

    for (std::int32_t row = 0; row < 4; ++row) {
        if (!new_inventory
                 .place(
                     makeInventoryItem(*tablet),
                     0,
                     row)
                 .accepted ||
            !new_inventory
                 .place(
                     makeInventoryItem(*capsule),
                     1,
                     row)
                 .accepted ||
            !new_belt
                 .place(
                     makeInventoryItem(*tablet),
                     row,
                     0,
                     *tablet)
                 .accepted ||
            !new_belt
                 .place(
                     makeInventoryItem(*capsule),
                     row,
                     1,
                     *capsule)
                 .accepted) {
            setError(
                error,
                "The initial medicine layout could not be created.");
            return false;
        }
    }

    inventory = std::move(new_inventory);
    equipment = std::move(new_equipment);
    belt = std::move(new_belt);
    return true;
}

}  // namespace osf
