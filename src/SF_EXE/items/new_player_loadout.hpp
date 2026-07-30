#ifndef OPENSHADOWFLARE_NEW_PLAYER_LOADOUT_HPP
#define OPENSHADOWFLARE_NEW_PLAYER_LOADOUT_HPP

#include <cstdint>
#include <string>

namespace osf {

class ItemDatabase;
class PlayerBelt;
class PlayerEquipment;
class PlayerInventory;

bool initializeRetailNewPlayerLoadout(
    const ItemDatabase& item_database,
    PlayerInventory& inventory,
    PlayerEquipment& equipment,
    PlayerBelt& belt,
    std::int32_t player_level,
    std::string* error = nullptr);

}  // namespace osf

#endif
