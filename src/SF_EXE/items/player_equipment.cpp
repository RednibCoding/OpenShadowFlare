#include "player_equipment.hpp"

#include "item_appearance.hpp"
#include "item_condition.hpp"
#include "item_database.hpp"
#include "item_instance_values.hpp"
#include "item_repair.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace osf {
namespace {

std::int32_t wrappedAdd(
    std::int32_t left,
    std::int32_t right) {
    const std::uint32_t value =
        static_cast<std::uint32_t>(left) +
        static_cast<std::uint32_t>(right);
    constexpr std::uint32_t kSignedMaximum =
        static_cast<std::uint32_t>(
            std::numeric_limits<std::int32_t>::max());
    return value <= kSignedMaximum
        ? static_cast<std::int32_t>(value)
        : -1 -
              static_cast<std::int32_t>(
                  std::numeric_limits<std::uint32_t>::max() -
                  value);
}

bool offHandIsSuppressed(
    const PlayerEquipment& equipment,
    const ItemDatabase& database) {
    const InventoryItem* main_hand =
        equipment.item(EquipmentSlot::main_hand);
    const ItemDefinition* definition =
        main_hand
            ? database.find(
                  main_hand->category,
                  main_hand->definition_id)
            : nullptr;
    return definition && definition->suppresses_off_hand;
}

}  // namespace

void PlayerEquipment::clear() {
    for (auto& slot : slots_) {
        slot.reset();
    }
}

EquipmentPlacementResult PlayerEquipment::place(
    EquipmentSlot slot,
    InventoryItem item,
    const ItemDefinition& definition,
    std::int32_t player_level) {
    // FUN_00446320 classifies the five ordinary and four accessory
    // regions, then applies the same required-level gate to each.
    if (!accepts(slot, definition) ||
        definition.id != item.definition_id ||
        item.category != definition.category ||
        item.quantity != 1 ||
        player_level < definition.required_level) {
        return {};
    }

    EquipmentPlacementResult result;
    result.accepted = true;
    std::optional<InventoryItem>& destination =
        slots_[static_cast<std::size_t>(slot)];
    result.held_item = std::move(destination);
    item.grid_x = 0;
    item.grid_y = 0;
    destination = std::move(item);
    return result;
}

std::optional<InventoryItem> PlayerEquipment::take(
    EquipmentSlot slot) {
    const std::size_t index =
        static_cast<std::size_t>(slot);
    if (index >= slot_count) {
        return std::nullopt;
    }
    std::optional<InventoryItem>& source =
        slots_[index];
    std::optional<InventoryItem> item =
        std::move(source);
    source.reset();
    return item;
}

const InventoryItem* PlayerEquipment::item(
    EquipmentSlot slot) const {
    const std::size_t index =
        static_cast<std::size_t>(slot);
    if (index >= slot_count) {
        return nullptr;
    }
    const std::optional<InventoryItem>& equipped =
        slots_[index];
    return equipped ? &*equipped : nullptr;
}

std::int32_t PlayerEquipment::appearanceColor(
    EquipmentSlot slot) const {
    const InventoryItem* equipped = item(slot);
    return equipped ? retailItemColorIndex(*equipped) : -1;
}

bool PlayerEquipment::setAppearanceColor(
    EquipmentSlot slot,
    std::int32_t color_index) {
    const std::size_t index = static_cast<std::size_t>(slot);
    return index < slot_count && slots_[index] &&
           setRetailItemColorIndex(*slots_[index], color_index);
}

bool PlayerEquipment::decreaseDurability(
    EquipmentSlot slot,
    std::int32_t amount) {
    const std::size_t index =
        static_cast<std::size_t>(slot);
    if (index >= slot_count || !slots_[index]) {
        return true;
    }
    InventoryItem& equipped = *slots_[index];
    if (amount > 0) {
        equipped.durability =
            equipped.durability <= amount
                ? 0
                : equipped.durability - amount;
    }
    return equipped.durability != 0;
}

std::int32_t PlayerEquipment::identifyAll() {
    std::int32_t identified = 0;
    for (std::size_t index = 0;
         index < visible_slot_count;
         ++index) {
        std::optional<InventoryItem>& item = slots_[index];
        if (item) {
            identified += identifyInventoryItem(*item) ? 1 : 0;
        }
    }
    return identified;
}

bool PlayerEquipment::hasUnidentifiedItems() const {
    return std::any_of(
        slots_.begin(),
        slots_.begin() +
            static_cast<std::ptrdiff_t>(visible_slot_count),
        [](const std::optional<InventoryItem>& item) {
            return item && item->identified == 0;
        });
}

std::int32_t PlayerEquipment::repairPrice(
    EquipmentRepairGroup group,
    const ItemDatabase& database,
    const TableData& value_parameters) const {
    const auto price = [this, &database, &value_parameters](
                           EquipmentSlot slot) {
        const InventoryItem* owned = item(slot);
        const ItemDefinition* definition = owned
            ? database.find(
                  owned->category, owned->definition_id)
            : nullptr;
        return definition
            ? retailItemRepairPrice(
                  *owned, *definition, value_parameters)
            : 0;
    };
    switch (group) {
    case EquipmentRepairGroup::arms:
        return wrappedAdd(
            price(EquipmentSlot::main_hand),
            price(EquipmentSlot::alternate_main_hand));
    case EquipmentRepairGroup::helmet:
        return price(EquipmentSlot::helmet);
    case EquipmentRepairGroup::body:
        return price(EquipmentSlot::body);
    case EquipmentRepairGroup::shields:
        return wrappedAdd(
            price(EquipmentSlot::off_hand),
            price(EquipmentSlot::alternate_off_hand));
    case EquipmentRepairGroup::boots:
        return price(EquipmentSlot::boots);
    }
    return 0;
}

std::int32_t PlayerEquipment::repair(
    EquipmentRepairGroup group,
    const ItemDatabase& database) {
    const auto repair_slot = [this, &database](EquipmentSlot slot) {
        std::optional<InventoryItem>& owned =
            slots_[static_cast<std::size_t>(slot)];
        const ItemDefinition* definition = owned
            ? database.find(
                  owned->category, owned->definition_id)
            : nullptr;
        if (!definition ||
            itemCurrentDurability(*owned, *definition) ==
                definition->maximum_durability) {
            return 0;
        }
        return repairInventoryItem(*owned, *definition) ? 1 : 0;
    };
    switch (group) {
    case EquipmentRepairGroup::arms:
        return repair_slot(EquipmentSlot::main_hand) +
               repair_slot(EquipmentSlot::alternate_main_hand);
    case EquipmentRepairGroup::helmet:
        return repair_slot(EquipmentSlot::helmet);
    case EquipmentRepairGroup::body:
        return repair_slot(EquipmentSlot::body);
    case EquipmentRepairGroup::shields:
        return repair_slot(EquipmentSlot::off_hand) +
               repair_slot(EquipmentSlot::alternate_off_hand);
    case EquipmentRepairGroup::boots:
        return repair_slot(EquipmentSlot::boots);
    }
    return 0;
}

std::int32_t PlayerEquipment::totalWeight(
    const ItemDatabase& database) const {
    std::int32_t weight = 0;
    const bool suppress_off_hand =
        offHandIsSuppressed(*this, database);
    for (std::size_t index = 0;
         index < visible_slot_count;
         ++index) {
        if (index ==
                static_cast<std::size_t>(
                    EquipmentSlot::off_hand) &&
            suppress_off_hand) {
            continue;
        }
        const auto& equipped = slots_[index];
        if (!equipped) {
            continue;
        }
        const ItemDefinition* definition =
            database.find(
                equipped->category,
                equipped->definition_id);
        if (definition) {
            weight +=
                definition->weight * equipped->quantity;
        }
    }
    return weight;
}

std::int32_t PlayerEquipment::derivedParameterBonus(
    std::size_t parameter,
    const ItemDatabase& database) const {
    if (parameter >= ItemDefinition::derived_parameter_count) {
        return 0;
    }
    return derivedParameterBonuses(database)[parameter];
}

PlayerEquipment::DerivedParameterBonuses
PlayerEquipment::derivedParameterBonuses(
    const ItemDatabase& database) const {
    static_assert(
        derived_parameter_count ==
        ItemDefinition::derived_parameter_count);
    DerivedParameterBonuses bonuses{};
    const bool suppress_off_hand =
        offHandIsSuppressed(*this, database);
    for (std::size_t index = 0;
         index < visible_slot_count;
         ++index) {
        if (index ==
                static_cast<std::size_t>(
                    EquipmentSlot::off_hand) &&
            suppress_off_hand) {
            continue;
        }
        const auto& equipped = slots_[index];
        if (!equipped ||
            (equipped->category <= 1 &&
             equipped->durability == 0)) {
            continue;
        }
        const ItemDefinition* definition =
            database.find(
                equipped->category,
                equipped->definition_id);
        if (definition) {
            for (std::size_t parameter = 0;
                 parameter < bonuses.size();
                 ++parameter) {
                bonuses[parameter] = wrappedAdd(
                    bonuses[parameter],
                    definition->derived_parameter_bonuses[
                        parameter]);
            }
        }
    }
    return bonuses;
}

std::int32_t PlayerEquipment::instanceParameterBonus(
    std::size_t parameter,
    const ItemDatabase& database) const {
    std::int32_t bonus = 0;
    const bool suppress_off_hand =
        offHandIsSuppressed(*this, database);
    for (std::size_t index = 0;
         index < visible_slot_count;
         ++index) {
        if (index ==
                static_cast<std::size_t>(
                    EquipmentSlot::off_hand) &&
            suppress_off_hand) {
            continue;
        }
        const auto& equipped = slots_[index];
        if (!equipped) {
            continue;
        }
        bonus = wrappedAdd(
            bonus,
            retailItemInstanceParameter(
                *equipped, parameter));
    }
    return bonus;
}

std::array<std::int32_t, 2>
PlayerEquipment::conditionalInstanceParameterBonus(
    std::size_t condition_parameter,
    std::size_t value_parameter,
    const ItemDatabase& database) const {
    std::array<std::int32_t, 2> result{};
    const bool suppress_off_hand =
        offHandIsSuppressed(*this, database);
    for (std::size_t index = 0;
         index < visible_slot_count;
         ++index) {
        if (index ==
                static_cast<std::size_t>(
                    EquipmentSlot::off_hand) &&
            suppress_off_hand) {
            continue;
        }
        const auto& equipped = slots_[index];
        if (!equipped) {
            continue;
        }
        const std::int32_t condition =
            retailItemInstanceParameter(
                *equipped, condition_parameter);
        result[0] = wrappedAdd(result[0], condition);
        if (condition != 0) {
            result[1] = wrappedAdd(
                result[1],
                retailItemInstanceParameter(
                    *equipped, value_parameter));
        }
    }
    return result;
}

bool PlayerEquipment::accepts(
    EquipmentSlot slot,
    const ItemDefinition& definition) {
    switch (slot) {
    case EquipmentSlot::main_hand:
    case EquipmentSlot::alternate_main_hand:
        return definition.category == 0;
    case EquipmentSlot::accessory_1:
    case EquipmentSlot::accessory_2:
    case EquipmentSlot::accessory_3:
    case EquipmentSlot::accessory_4:
        return definition.category == 2 &&
               definition.inventory_width == 1;
    case EquipmentSlot::helmet:
        return definition.category == 1 &&
               definition.subtype == 0;
    case EquipmentSlot::body:
        return definition.category == 1 &&
               definition.subtype == 1;
    case EquipmentSlot::boots:
        return definition.category == 1 &&
               definition.subtype == 3;
    case EquipmentSlot::off_hand:
    case EquipmentSlot::alternate_off_hand:
        return definition.category == 1 &&
               definition.subtype == 2;
    case EquipmentSlot::count:
        return false;
    }
    return false;
}

}  // namespace osf
