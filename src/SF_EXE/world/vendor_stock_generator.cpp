#include "vendor_stock_generator.hpp"

#include "core/retail_random.hpp"
#include "items/item_database.hpp"
#include "items/item_instance_factory.hpp"
#include "items/vendor_inventory.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace osf {
namespace {

const ItemDefinition* chooseDefinition(
    std::int32_t category,
    std::int32_t maximum_level,
    std::int32_t minimum_level,
    const std::array<bool, 4>& variants,
    const ItemDatabase& database,
    RetailRandom& random) {
    std::vector<const ItemDefinition*> candidates;
    std::int64_t total_weight = 0;
    for (std::size_t candidate_category = 0;
         candidate_category < 3;
         ++candidate_category) {
        if (category >= 0 &&
            candidate_category !=
                static_cast<std::size_t>(category)) {
            continue;
        }
        for (const ItemDefinition& definition :
             database.definitions(candidate_category)) {
            if (definition.variant < 0 ||
                definition.variant >= 4 ||
                !variants[static_cast<std::size_t>(
                    definition.variant)] ||
                (definition.loot_episode_mask & 1) == 0 ||
                (maximum_level >= 0 &&
                 definition.loot_level > maximum_level) ||
                (minimum_level >= 0 &&
                 definition.loot_level < minimum_level) ||
                definition.loot_weight <= 0) {
                continue;
            }
            candidates.push_back(&definition);
            total_weight += definition.loot_weight;
        }
    }
    if (candidates.empty() || total_weight <= 0) {
        return nullptr;
    }
    std::int64_t roll = random.next() % total_weight;
    for (const ItemDefinition* definition : candidates) {
        if (roll < definition->loot_weight) {
            return definition;
        }
        roll -= definition->loot_weight;
    }
    return candidates.back();
}

bool addStockEntry(
    VendorInventory& inventory,
    const TableData& entry_table,
    std::int32_t entry_row,
    const ItemDatabase& database,
    RetailRandom& random) {
    if (!entry_table.contains(entry_row, 9)) {
        return false;
    }
    const std::int32_t category =
        entry_table.value(entry_row, 0);
    const std::int32_t fixed_definition =
        entry_table.value(entry_row, 3);
    std::int32_t count = entry_table.value(entry_row, 4);
    if (count < 0) {
        count = random.next() % 30;
    }
    const std::array<bool, 4> variants = {
        entry_table.value(entry_row, 7) != 0,
        entry_table.value(entry_row, 8) != 0,
        entry_table.value(entry_row, 9) != 0,
        true,
    };
    for (std::int32_t copy = 0; copy < count; ++copy) {
        const ItemDefinition* definition = nullptr;
        if (fixed_definition >= 0 && category >= 0) {
            definition = database.find(category, fixed_definition);
        } else {
            definition = chooseDefinition(
                category,
                entry_table.value(entry_row, 1),
                entry_table.value(entry_row, 2),
                variants,
                database,
                random);
        }
        if (!definition) {
            continue;
        }
        InventoryItem item = makeRetailInventoryItem(
            *definition,
            [&random]() { return random.next(); });
        const std::int32_t start_x =
            entry_table.value(entry_row, 5);
        const std::int32_t start_y =
            entry_table.value(entry_row, 6);
        inventory.store(
            std::move(item), start_x, start_y);
    }
    return true;
}

}  // namespace

bool generateRetailVendorStock(
    VendorInventory& inventory,
    std::int32_t profile,
    const TableDatabase& tables,
    const ItemDatabase& items,
    RetailRandom& random) {
    const TableData* profiles = tables.find(32);
    const TableData* entries = tables.find(33);
    if (!profiles || !entries || profile < 0 ||
        profile >= profiles->rowCount()) {
        return false;
    }
    VendorInventory generated;
    for (std::int32_t column = 0;
         column < profiles->columnCount();
         ++column) {
        const std::int32_t entry =
            profiles->value(profile, column);
        if (entry >= 0 &&
            !addStockEntry(
                generated, *entries, entry, items, random)) {
            return false;
        }
    }
    inventory = std::move(generated);
    return true;
}

}  // namespace osf
