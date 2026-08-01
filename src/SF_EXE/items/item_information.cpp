#include "item_information.hpp"

#include "item_condition.hpp"
#include "item_database.hpp"
#include "player_inventory.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace osf {
namespace {

constexpr std::array<
    std::string_view,
    ItemDefinition::derived_parameter_count> kParameterLabels = {
        "Attack                    :",
        "Hit Rate                  :",
        "Defense                   :",
        "Evasion Rate              :",
        "Magical Attack            :",
        "Magical Hit Rate          :",
        "Magical Defense           :",
        "Magical Evasion Rate      :",
        "Speed of Attack           :",
        "Walking Speed             :",
};

void appendValue(
    std::ostringstream& output,
    std::string_view label,
    std::int32_t value) {
    output << label << std::setw(9) << value << '\n';
}

}  // namespace

std::int32_t itemSalePrice(
    const InventoryItem& item,
    const ItemDefinition& definition) {
    std::int64_t price =
        std::max<std::int32_t>(definition.base_price, 0);
    if ((definition.category == 0 ||
         definition.category == 1) &&
        definition.maximum_durability > 0) {
        price =
            price *
            std::clamp<std::int32_t>(
                itemCurrentDurability(item, definition),
                0,
                definition.maximum_durability) /
            definition.maximum_durability;
    }
    price /= 4;
    return static_cast<std::int32_t>(
        std::max<std::int64_t>(price, 1));
}

std::string itemInformationText(
    const InventoryItem& item,
    const ItemDefinition& definition) {
    std::ostringstream output;
    output << '[' << definition.name << "]\n\n";

    if (definition.category == 0 ||
        definition.category == 1) {
        for (std::size_t parameter = 0;
             parameter <
                 definition.derived_parameter_bonuses.size();
             ++parameter) {
            const std::int32_t value =
                definition.derived_parameter_bonuses[parameter];
            if (value != 0) {
                appendValue(
                    output,
                    kParameterLabels[parameter],
                    value);
            }
        }
        appendValue(
            output,
            "Durability                :",
            itemCurrentDurability(item, definition));
        appendValue(
            output,
            "Weight                    :",
            definition.weight);
        appendValue(
            output,
            "Required Level            :",
            definition.required_level);
        appendValue(
            output,
            "Sale Price                :",
            itemSalePrice(item, definition));
        output << '\n';
        output
            << "Fire   :" << std::setw(3)
            << definition.element_strengths[0]
            << " Water  :" << std::setw(3)
            << definition.element_strengths[1]
            << " Earth  :" << std::setw(3)
            << definition.element_strengths[2]
            << " Thunder:" << std::setw(3)
            << definition.element_strengths[3] << '\n'
            << "Holy   :" << std::setw(3)
            << definition.element_strengths[4]
            << " Dark   :" << std::setw(3)
            << definition.element_strengths[5]
            << " Gel    :" << std::setw(3)
            << definition.element_strengths[6]
            << " Metal  :" << std::setw(3)
            << definition.element_strengths[7] << '\n';
    } else if (
        definition.category == 4 &&
        definition.id == 0) {
        appendValue(
            output,
            "Price                     :",
            item.quantity);
    }
    return output.str();
}

}  // namespace osf
