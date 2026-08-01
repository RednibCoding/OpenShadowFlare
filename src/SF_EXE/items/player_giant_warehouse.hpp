#ifndef OPENSHADOWFLARE_PLAYER_GIANT_WAREHOUSE_HPP
#define OPENSHADOWFLARE_PLAYER_GIANT_WAREHOUSE_HPP

#include "player_special_items.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace osf {

class PlayerGiantWarehouse {
public:
    static constexpr std::size_t page_count = 10;
    using EnabledFlags =
        std::array<std::int32_t, page_count>;

    void initializeNew();
    void restoreEnabledFlags(const EnabledFlags& flags);

    bool pageEnabled(std::size_t page) const;
    const EnabledFlags& enabledFlags() const;
    std::size_t selectedPage() const;
    bool selectPage(std::size_t page);

    PlayerSpecialItems& page(std::size_t page);
    const PlayerSpecialItems& page(std::size_t page) const;

private:
    std::array<PlayerSpecialItems, page_count> pages_{};
    EnabledFlags enabled_flags_{};
    std::size_t selected_page_ = 0;
};

}  // namespace osf

#endif
