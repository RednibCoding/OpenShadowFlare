#include "item_instance_factory.hpp"

#include "item_database.hpp"

#include <cstddef>
#include <cstdint>

namespace osf {
namespace {

constexpr std::size_t kWordSize = 4;
constexpr std::size_t kRolledElementWord = 39;

void writeWord(
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

std::int32_t rollValue(
    const RetailItemRoll& roll,
    bool always_consume_range,
    const ItemRandomSource& random) {
    if (random() % 100 >= roll.chance) {
        return 0;
    }
    const std::int64_t range =
        static_cast<std::int64_t>(roll.maximum) -
        roll.minimum + 1;
    if (range <= 0) {
        return roll.minimum;
    }
    if (!always_consume_range &&
        roll.minimum == roll.maximum) {
        return roll.minimum;
    }
    return roll.minimum +
        random() % static_cast<std::int32_t>(range);
}

std::size_t retailStateWords(std::int32_t category) {
    switch (category) {
    case 0:
    case 1:
        return 50;
    case 2:
        return 48;
    case 3:
        return 0;
    case 4:
        return 1;
    default:
        return 0;
    }
}

}  // namespace

InventoryItem makeRetailInventoryItem(
    const ItemDefinition& definition,
    const ItemRandomSource& random,
    std::int32_t quantity) {
    InventoryItem item =
        makeInventoryItem(definition, quantity);
    item.retail_state.resize(
        retailStateWords(definition.category) *
        kWordSize);

    if (definition.category <= 2) {
        const bool always_consume_range =
            definition.category != 0;
        for (std::size_t parameter = 0;
             parameter <
                 definition.instance_parameter_rolls.size();
             ++parameter) {
            writeWord(
                item.retail_state,
                parameter,
                rollValue(
                    definition
                        .instance_parameter_rolls[parameter],
                    always_consume_range,
                    random));
        }
        for (std::size_t element = 0;
             element < definition.element_rolls.size();
             ++element) {
            writeWord(
                item.retail_state,
                kRolledElementWord + element,
                rollValue(
                    definition.element_rolls[element],
                    always_consume_range,
                    random));
        }
    }

    if (definition.category == 0 ||
        definition.category == 1) {
        writeWord(item.retail_state, 47, item.durability);
        writeWord(item.retail_state, 48, item.identified);
        writeWord(item.retail_state, 49, -1);
    } else if (definition.category == 2) {
        writeWord(item.retail_state, 47, item.identified);
    } else if (definition.category == 4) {
        writeWord(item.retail_state, 0, item.quantity);
    }
    return item;
}

}  // namespace osf
