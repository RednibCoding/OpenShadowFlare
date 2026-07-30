#ifndef OPENSHADOWFLARE_PLAYER_ITEM_CONTROLLER_HPP
#define OPENSHADOWFLARE_PLAYER_ITEM_CONTROLLER_HPP

#include <cstdint>

namespace osf {

class ItemDatabase;
class PlayerBelt;
class PlayerData;

struct BeltItemUseResult {
    bool consumed = false;
    std::int32_t sound_sample = -1;
};

class PlayerItemController {
public:
    void clear();
    void initializeNew();

    BeltItemUseResult useBeltPocket(
        std::int32_t pocket,
        PlayerBelt& belt,
        const ItemDatabase& item_database,
        PlayerData& player);

    std::int32_t mineCount() const;

private:
    std::int32_t mine_count_ = 0;
};

}  // namespace osf

#endif
