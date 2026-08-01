#ifndef OPENSHADOWFLARE_PLAYER_AUTOMATIC_ITEMS_HPP
#define OPENSHADOWFLARE_PLAYER_AUTOMATIC_ITEMS_HPP

#include "player_special_items.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace osf {

struct ItemDefinition;

// Retail keeps category-four records with an authored page number out of the
// backpack. They occupy a fixed cell in one of these four private pages and
// are the first item owners searched by scenario commands 58 and 59.
class PlayerAutomaticItems {
public:
    static constexpr std::size_t page_count = 4;

    void clear();
    bool add(
        const ItemDefinition& definition,
        InventoryItem item);
    bool contains(
        std::int32_t category,
        std::int32_t definition_id) const;
    bool removeFirst(
        std::int32_t category,
        std::int32_t definition_id);

    PlayerSpecialItems& page(std::size_t page);
    const PlayerSpecialItems& page(std::size_t page) const;

private:
    std::array<PlayerSpecialItems, page_count> pages_{};
};

}  // namespace osf

#endif
