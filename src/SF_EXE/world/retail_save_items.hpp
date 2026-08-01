#ifndef OPENSHADOWFLARE_RETAIL_SAVE_ITEMS_HPP
#define OPENSHADOWFLARE_RETAIL_SAVE_ITEMS_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace osf {

class ItemDatabase;
class PlayerBelt;
class PlayerEquipment;
class PlayerInventory;
class PlayerSpecialItems;

bool restoreRetailOwnedItems(
    const std::vector<std::uint8_t>& payload,
    const ItemDatabase& item_database,
    std::int32_t player_level,
    PlayerInventory& inventory,
    PlayerEquipment& equipment,
    PlayerBelt& belt,
    PlayerSpecialItems& special_items,
    std::size_t* serialized_end = nullptr,
    std::string* error = nullptr);

bool replaceRetailOwnedItems(
    std::vector<std::uint8_t>& payload,
    const ItemDatabase& item_database,
    const PlayerInventory& inventory,
    const PlayerEquipment& equipment,
    const PlayerBelt& belt,
    const PlayerSpecialItems& special_items,
    std::size_t* serialized_end = nullptr,
    std::string* error = nullptr);

// Shared codec for the later warehouse owners in the same retail stream.
// These helpers deliberately expose a complete 9x10 container rather than
// the item-record internals so every owner keeps the same validation rules.
bool restoreRetailSpecialItemContainer(
    const std::vector<std::uint8_t>& payload,
    std::size_t offset,
    const ItemDatabase& item_database,
    PlayerSpecialItems& items,
    std::size_t* serialized_end = nullptr,
    std::string* error = nullptr);

bool appendRetailSpecialItemContainer(
    std::vector<std::uint8_t>& payload,
    const ItemDatabase& item_database,
    const PlayerSpecialItems& items,
    std::string* error = nullptr);

}  // namespace osf

#endif
