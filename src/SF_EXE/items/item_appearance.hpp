#ifndef OPENSHADOWFLARE_ITEM_APPEARANCE_HPP
#define OPENSHADOWFLARE_ITEM_APPEARANCE_HPP

#include <cstdint>

namespace osf {

struct InventoryItem;

struct ItemAppearanceStrength {
    std::int32_t red = 1000;
    std::int32_t green = 1000;
    std::int32_t blue = 1000;
};

constexpr std::int32_t retail_item_color_count = 16;

std::int32_t retailItemColorIndex(const InventoryItem& item);
bool setRetailItemColorIndex(
    InventoryItem& item,
    std::int32_t color_index);
ItemAppearanceStrength retailItemColorStrength(
    std::int32_t color_index);

}  // namespace osf

#endif
