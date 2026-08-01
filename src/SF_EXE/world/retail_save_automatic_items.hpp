#ifndef OPENSHADOWFLARE_RETAIL_SAVE_AUTOMATIC_ITEMS_HPP
#define OPENSHADOWFLARE_RETAIL_SAVE_AUTOMATIC_ITEMS_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace osf {

class ItemDatabase;
class PlayerAutomaticItems;

bool restoreRetailAutomaticItems(
    const std::vector<std::uint8_t>& payload,
    std::size_t giant_warehouse_end,
    const ItemDatabase& item_database,
    PlayerAutomaticItems& items,
    std::size_t* serialized_end = nullptr,
    std::string* error = nullptr);

bool replaceRetailAutomaticItems(
    std::vector<std::uint8_t>& payload,
    std::size_t giant_warehouse_end,
    const ItemDatabase& item_database,
    const PlayerAutomaticItems& items,
    std::size_t* serialized_end = nullptr,
    std::string* error = nullptr);

}  // namespace osf

#endif
