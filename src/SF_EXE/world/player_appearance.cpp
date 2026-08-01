#include "player_appearance.hpp"

#include "items/item_appearance.hpp"
#include "items/item_database.hpp"
#include "items/player_equipment.hpp"

namespace osf {

void PlayerAppearance::clear() {
    enabled_.clear();
    red_strengths_.clear();
    green_strengths_.clear();
    blue_strengths_.clear();
}

void PlayerAppearance::refresh(
    std::size_t part_count,
    const PlayerEquipment& equipment,
    const ItemDatabase& item_database) {
    enabled_.assign(part_count, 0);
    red_strengths_.assign(part_count, 1000);
    green_strengths_.assign(part_count, 1000);
    blue_strengths_.assign(part_count, 1000);
    if (!enabled_.empty()) {
        enabled_[0] = 1;
    }
    if (enabled_.size() > 1) {
        enabled_[1] = 1;
    }

    const auto definitionForSlot =
        [&equipment, &item_database](
            EquipmentSlot slot) -> const ItemDefinition* {
        const InventoryItem* equipped = equipment.item(slot);
        if (!equipped) {
            return nullptr;
        }
        return item_database.find(
            equipped->category,
            equipped->definition_id);
    };
    const auto enablePart =
        [this, part_count](
            std::int32_t part_value,
            std::int32_t red,
            std::int32_t green,
            std::int32_t blue) {
        if (part_value < 0 ||
            static_cast<std::size_t>(part_value) >= part_count) {
            return;
        }
        const std::size_t part =
            static_cast<std::size_t>(part_value);
        enabled_[part] = 1;
        red_strengths_[part] = red;
        green_strengths_[part] = green;
        blue_strengths_[part] = blue;
    };
    const auto enablePrimary =
        [&enablePart, &equipment](
            EquipmentSlot slot,
            const ItemDefinition* definition) {
        if (definition) {
            ItemAppearanceStrength strength{
                definition->appearance_red_strength,
                definition->appearance_green_strength,
                definition->appearance_blue_strength,
            };
            const std::int32_t color =
                equipment.appearanceColor(slot);
            if (color >= 0) {
                strength = retailItemColorStrength(color);
            }
            enablePart(
                definition->appearance_part,
                strength.red,
                strength.green,
                strength.blue);
        }
    };

    // FUN_00444ca0 contributes body, weapon, and off-hand objects to the
    // player CAF mask. Helmets and boots remain stat-bearing only.
    enablePrimary(
        EquipmentSlot::body,
        definitionForSlot(EquipmentSlot::body));
    const ItemDefinition* main_hand =
        definitionForSlot(EquipmentSlot::main_hand);
    enablePrimary(EquipmentSlot::main_hand, main_hand);
    if (main_hand) {
        enablePart(
            main_hand->secondary_appearance_part,
            main_hand->secondary_appearance_red_strength,
            main_hand->secondary_appearance_green_strength,
            main_hand->secondary_appearance_blue_strength);
    }
    if (!main_hand ||
        !main_hand->suppresses_off_hand) {
        enablePrimary(
            EquipmentSlot::off_hand,
            definitionForSlot(EquipmentSlot::off_hand));
    }
}

bool PlayerAppearance::partEnabled(std::size_t part) const {
    return part < enabled_.size() && enabled_[part] != 0;
}

std::int32_t PlayerAppearance::redStrength(
    std::size_t part) const {
    return part < red_strengths_.size()
        ? red_strengths_[part]
        : 1000;
}

std::int32_t PlayerAppearance::greenStrength(
    std::size_t part) const {
    return part < green_strengths_.size()
        ? green_strengths_[part]
        : 1000;
}

std::int32_t PlayerAppearance::blueStrength(
    std::size_t part) const {
    return part < blue_strengths_.size()
        ? blue_strengths_[part]
        : 1000;
}

}  // namespace osf
