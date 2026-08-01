#include "item_appearance.hpp"

#include "player_inventory.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace osf {
namespace {

constexpr std::size_t kColorWord = 49;
constexpr std::size_t kDurabilityWord = 47;
constexpr std::size_t kIdentifiedWord = 48;
constexpr std::size_t kWordSize = 4;
constexpr std::size_t kColorItemStateSize = 50 * kWordSize;

constexpr std::array<
    ItemAppearanceStrength,
    retail_item_color_count> kRetailColors{{
    {800, 250, 250},
    {950, 850, 0},
    {550, 850, 250},
    {1000, 1000, 1000},
    {1050, 600, 300},
    {1050, 750, 0},
    {400, 600, 400},
    {750, 750, 750},
    {550, 1000, 900},
    {250, 500, 850},
    {850, 650, 1000},
    {500, 500, 500},
    {250, 750, 650},
    {400, 400, 600},
    {1000, 400, 600},
    {300, 300, 300},
}};

std::int32_t readI32(
    const std::vector<std::uint8_t>& bytes,
    std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        return -1;
    }
    const std::uint32_t value =
        static_cast<std::uint32_t>(bytes[offset]) |
        (static_cast<std::uint32_t>(bytes[offset + 1]) << 8u) |
        (static_cast<std::uint32_t>(bytes[offset + 2]) << 16u) |
        (static_cast<std::uint32_t>(bytes[offset + 3]) << 24u);
    return static_cast<std::int32_t>(value);
}

void writeI32(
    std::vector<std::uint8_t>& bytes,
    std::size_t offset,
    std::int32_t value) {
    const std::uint32_t data = static_cast<std::uint32_t>(value);
    bytes[offset] = static_cast<std::uint8_t>(data);
    bytes[offset + 1] = static_cast<std::uint8_t>(data >> 8u);
    bytes[offset + 2] = static_cast<std::uint8_t>(data >> 16u);
    bytes[offset + 3] = static_cast<std::uint8_t>(data >> 24u);
}

}  // namespace

std::int32_t retailItemColorIndex(const InventoryItem& item) {
    if ((item.category != 0 && item.category != 1) ||
        item.retail_state.size() < (kColorWord + 1) * kWordSize) {
        return -1;
    }
    const std::int32_t color =
        readI32(item.retail_state, kColorWord * kWordSize);
    return color >= 0 && color < retail_item_color_count
        ? color
        : -1;
}

bool setRetailItemColorIndex(
    InventoryItem& item,
    std::int32_t color_index) {
    if ((item.category != 0 && item.category != 1) ||
        (color_index < -1 || color_index >= retail_item_color_count)) {
        return false;
    }
    if (item.retail_state.empty()) {
        item.retail_state.resize(kColorItemStateSize);
        writeI32(
            item.retail_state,
            kDurabilityWord * kWordSize,
            item.durability);
        writeI32(
            item.retail_state,
            kIdentifiedWord * kWordSize,
            item.identified);
        writeI32(
            item.retail_state,
            kColorWord * kWordSize,
            -1);
    } else if (item.retail_state.size() != kColorItemStateSize) {
        return false;
    }
    writeI32(
        item.retail_state,
        kColorWord * kWordSize,
        color_index);
    return true;
}

ItemAppearanceStrength retailItemColorStrength(
    std::int32_t color_index) {
    return color_index >= 0 && color_index < retail_item_color_count
        ? kRetailColors[static_cast<std::size_t>(color_index)]
        : ItemAppearanceStrength{};
}

}  // namespace osf
