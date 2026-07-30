#include "item_instance_values.hpp"

#include "item_database.hpp"
#include "player_inventory.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>

namespace osf {
namespace {

constexpr std::size_t kRolledElementWord = 39;
constexpr std::size_t kWordSize = 4;

std::int32_t signedWord(std::uint32_t value) {
    constexpr std::uint32_t kSignedMaximum =
        static_cast<std::uint32_t>(
            std::numeric_limits<std::int32_t>::max());
    if (value <= kSignedMaximum) {
        return static_cast<std::int32_t>(value);
    }
    return -1 -
           static_cast<std::int32_t>(
               std::numeric_limits<std::uint32_t>::max() -
               value);
}

std::int32_t readStateI32(
    const std::vector<std::uint8_t>& state,
    std::size_t offset) {
    if (offset > state.size() ||
        state.size() - offset < kWordSize) {
        return 0;
    }
    const std::uint32_t value =
        static_cast<std::uint32_t>(state[offset]) |
        (static_cast<std::uint32_t>(state[offset + 1]) << 8u) |
        (static_cast<std::uint32_t>(state[offset + 2]) << 16u) |
        (static_cast<std::uint32_t>(state[offset + 3]) << 24u);
    return signedWord(value);
}

}  // namespace

std::int32_t retailItemRolledElementStrength(
    const InventoryItem& item,
    std::size_t element) {
    if (element >= ItemDefinition::element_count ||
        item.category < 0 ||
        item.category > 2) {
        return 0;
    }
    return readStateI32(
        item.retail_state,
        (kRolledElementWord + element) * kWordSize);
}

std::int32_t retailItemInstanceParameter(
    const InventoryItem& item,
    std::size_t parameter) {
    if (parameter >= 39 ||
        item.category < 0 ||
        item.category > 2) {
        return 0;
    }
    return readStateI32(
        item.retail_state,
        parameter * kWordSize);
}

std::int32_t retailItemElementStrength(
    const InventoryItem& item,
    const ItemDefinition& definition,
    std::size_t element) {
    if (element >= ItemDefinition::element_count ||
        item.category != definition.category ||
        item.definition_id != definition.id) {
        return 0;
    }

    std::int32_t strength =
        retailItemRolledElementStrength(item, element);
    if (definition.category == 0 ||
        definition.category == 1) {
        strength = signedWord(
            static_cast<std::uint32_t>(
                definition.element_strengths[element]) +
            static_cast<std::uint32_t>(strength));
    }
    return strength;
}

}  // namespace osf
