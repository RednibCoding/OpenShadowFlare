#include "item_repair.hpp"

#include "item_condition.hpp"
#include "item_database.hpp"
#include "item_instance_values.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "player_inventory.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace osf {
namespace {

constexpr std::size_t kWordSize = 4;
constexpr std::size_t kDurabilityWord = 47;

std::int32_t signedWord(std::uint32_t value) {
    constexpr std::uint32_t kSignedMaximum = 0x7fffffffu;
    return value <= kSignedMaximum
        ? static_cast<std::int32_t>(value)
        : -1 - static_cast<std::int32_t>(0xffffffffu - value);
}

std::uint32_t magnitude(std::int32_t value) {
    const std::uint32_t encoded =
        static_cast<std::uint32_t>(value);
    return value < 0 ? 0u - encoded : encoded;
}

std::int32_t addWeightedValue(
    std::int32_t total,
    std::int32_t coefficient,
    std::int32_t value) {
    return signedWord(
        static_cast<std::uint32_t>(total) +
        static_cast<std::uint32_t>(coefficient) *
            magnitude(value));
}

void writeStateI32(
    std::vector<std::uint8_t>& state,
    std::size_t word,
    std::int32_t value) {
    const std::size_t offset = word * kWordSize;
    if (offset > state.size() ||
        state.size() - offset < kWordSize) {
        return;
    }
    const std::uint32_t encoded =
        static_cast<std::uint32_t>(value);
    state[offset] = static_cast<std::uint8_t>(encoded);
    state[offset + 1] =
        static_cast<std::uint8_t>(encoded >> 8u);
    state[offset + 2] =
        static_cast<std::uint8_t>(encoded >> 16u);
    state[offset + 3] =
        static_cast<std::uint8_t>(encoded >> 24u);
}

}  // namespace

std::int32_t retailItemValue(
    const InventoryItem& item,
    const ItemDefinition& definition,
    const TableData& value_parameters) {
    std::int32_t value = definition.base_price;
    if (item.category < 0 || item.category > 2 ||
        item.category != definition.category ||
        item.definition_id != definition.id) {
        return value;
    }

    for (std::size_t element = 0;
         element < ItemDefinition::element_count;
         ++element) {
        value = addWeightedValue(
            value,
            value_parameters.value(
                static_cast<std::int32_t>(element), 0),
            retailItemRolledElementStrength(item, element));
    }
    for (std::size_t parameter = 0;
         parameter < ItemDefinition::instance_parameter_count;
         ++parameter) {
        std::int32_t parameter_value =
            retailItemInstanceParameter(item, parameter);
        if (parameter == 16 && item.category == 0) {
            parameter_value = signedWord(
                1u - static_cast<std::uint32_t>(parameter_value));
        }
        value = addWeightedValue(
            value,
            value_parameters.value(
                static_cast<std::int32_t>(parameter + 8u), 0),
            parameter_value);
    }
    return value;
}

std::int32_t retailItemRepairPrice(
    const InventoryItem& item,
    const ItemDefinition& definition,
    const TableData& value_parameters) {
    if ((item.category != 0 && item.category != 1) ||
        item.category != definition.category ||
        item.definition_id != definition.id ||
        definition.maximum_durability <= 0) {
        return 0;
    }
    const std::int32_t current = std::clamp<std::int32_t>(
        itemCurrentDurability(item, definition),
        0,
        definition.maximum_durability);
    if (current == definition.maximum_durability) {
        return 0;
    }
    const std::int32_t repair_unit =
        retailItemValue(item, definition, value_parameters) / 10;
    const std::int32_t scaled = signedWord(
        static_cast<std::uint32_t>(
            definition.maximum_durability - current) *
        static_cast<std::uint32_t>(repair_unit));
    const std::int32_t price =
        scaled / definition.maximum_durability;
    return price == 0 ? 1 : price;
}

bool repairInventoryItem(
    InventoryItem& item,
    const ItemDefinition& definition) {
    if ((item.category != 0 && item.category != 1) ||
        item.category != definition.category ||
        item.definition_id != definition.id ||
        definition.maximum_durability <= 0) {
        return false;
    }
    item.durability = definition.maximum_durability;
    writeStateI32(
        item.retail_state,
        kDurabilityWord,
        item.durability);
    return true;
}

}  // namespace osf
