#ifndef OPENSHADOWFLARE_RETAIL_SAVE_GIANT_WAREHOUSE_HPP
#define OPENSHADOWFLARE_RETAIL_SAVE_GIANT_WAREHOUSE_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace osf {

class ItemDatabase;
class PlayerGiantWarehouse;

bool restoreRetailGiantWarehouse(
    const std::vector<std::uint8_t>& payload,
    std::size_t mine_end,
    const ItemDatabase& item_database,
    PlayerGiantWarehouse& warehouse,
    std::size_t* serialized_end = nullptr,
    std::string* error = nullptr);

bool replaceRetailGiantWarehouse(
    std::vector<std::uint8_t>& payload,
    std::size_t mine_end,
    const ItemDatabase& item_database,
    const PlayerGiantWarehouse& warehouse,
    std::size_t* serialized_end = nullptr,
    std::string* error = nullptr);

}  // namespace osf

#endif
