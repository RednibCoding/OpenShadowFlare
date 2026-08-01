#ifndef OPENSHADOWFLARE_PLAYER_ITEM_CONTROLLER_HPP
#define OPENSHADOWFLARE_PLAYER_ITEM_CONTROLLER_HPP

#include <cstdint>

namespace osf {

class ItemDatabase;
class PlayerBelt;
class PlayerData;
class PlayerInventory;

struct PlayerItemUseResult {
    bool consumed = false;
    std::int32_t sound_sample = -1;
};

class PlayerItemController {
public:
    void clear();
    void initializeNew();
    void restoreMineCount(std::int32_t count);
    bool consumeMine();
    bool collectMine(std::int32_t maximum_count);

    PlayerItemUseResult useBeltPocket(
        std::int32_t pocket,
        PlayerBelt& belt,
        const ItemDatabase& item_database,
        PlayerData& player);
    PlayerItemUseResult useInventoryItem(
        std::int32_t item_index,
        PlayerInventory& inventory,
        const ItemDatabase& item_database,
        PlayerData& player);

    std::int32_t mineCount() const;

private:
    std::int32_t mine_count_ = 0;
};

}  // namespace osf

#endif
