#include "core/retail_random.hpp"
#include "items/item_database.hpp"
#include "items/item_information.hpp"
#include "items/player_belt.hpp"
#include "items/player_equipment.hpp"
#include "items/player_inventory.hpp"
#include "items/player_special_items.hpp"
#include "items/vendor_inventory.hpp"
#include "libs/RKC_RPG_TABLE/rkc_rpg_table.hpp"
#include "states/gameplay_inventory.hpp"
#include "states/gameplay_vendor.hpp"
#include "world/vendor_stock_generator.hpp"

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool testVendorStockAndTransactions() {
    const std::filesystem::path data =
        std::filesystem::path(OPENSHADOWFLARE_SOURCE_DIR) /
        "tmp" / "ShadowFlare";
    osf::TableDatabase tables;
    osf::ItemDatabase database;
    std::string error;
    if (!check(
            tables.load(
                data / "System/Game/Parameter/Table.Tbd",
                &error) &&
                database.load(
                    data / "System/Game/Parameter/Item.Ibn",
                    &error),
            "The retail vendor parameter data could not be loaded.")) {
        return false;
    }

    osf::VendorInventory vendor;
    osf::RetailRandom random(1);
    if (!check(
            osf::generateRetailVendorStock(
                vendor, 0, tables, database, random) &&
                !vendor.items().empty(),
            "Retail vendor profile zero did not generate stock.")) {
        return false;
    }
    const osf::InventoryItem stock_item = vendor.items().front();
    const osf::ItemDefinition* definition = database.find(
        stock_item.category, stock_item.definition_id);
    if (!check(
            definition != nullptr &&
                stock_item.grid_x >= 0 &&
                stock_item.grid_y >= 0 &&
                stock_item.grid_x + stock_item.width <=
                    osf::VendorInventory::grid_width &&
                stock_item.grid_y + stock_item.height <=
                    osf::VendorInventory::grid_height,
            "Generated vendor stock does not fit the retail grid.")) {
        return false;
    }

    const std::int32_t price =
        osf::itemPurchasePrice(stock_item, *definition);
    if (price > osf::PlayerInventory::maximum_gold_stack) {
        return true;
    }
    osf::PlayerInventory player;
    if (!check(
            player.add(
                4,
                0,
                osf::PlayerInventory::maximum_gold_stack),
            "The vendor test gold could not be stored.")) {
        return false;
    }
    osf::GameplayInventory inventory;
    inventory.open();
    osf::GameplayVendor state;
    state.open(0);
    const osf::GameplayVendorResult picked = state.update(
        {
            false,
            true,
            osf::GameplayVendor::item_left +
                stock_item.grid_x *
                    osf::GameplayVendor::cell_size + 1,
            osf::GameplayVendor::item_top +
                stock_item.grid_y *
                    osf::GameplayVendor::cell_size + 1,
        },
        vendor,
        inventory,
        player,
        database);
    if (!check(
            picked.pointer_consumed &&
                inventory.heldItemFromVendor() &&
                player.gold() ==
                    osf::PlayerInventory::maximum_gold_stack,
            "Picking up vendor stock did not start a pending purchase.")) {
        return false;
    }

    osf::PlayerEquipment equipment;
    osf::PlayerBelt belt;
    osf::PlayerSpecialItems special;
    const osf::GameplayInventoryResult placed = inventory.update(
        {
            false,
            false,
            true,
            osf::GameplayInventory::backpack_left +
                (osf::PlayerInventory::grid_width -
                 stock_item.width) *
                    osf::GameplayInventory::cell_size +
                stock_item.width *
                    osf::GameplayInventory::cell_size / 2,
            osf::GameplayInventory::backpack_top +
                (osf::PlayerInventory::grid_height -
                 stock_item.height) *
                    osf::GameplayInventory::cell_size +
                stock_item.height *
                    osf::GameplayInventory::cell_size / 2,
        },
        player,
        equipment,
        belt,
        special,
        database,
        99);
    if (!check(
            placed.pointer_consumed &&
                !inventory.heldItemFromVendor() &&
                player.gold() ==
                    osf::PlayerInventory::maximum_gold_stack - price,
            "Completing a vendor purchase did not debit its retail "
            "price.")) {
        return false;
    }

    const osf::GameplayInventoryResult selected_for_sale =
        inventory.update(
            {
                false,
                false,
                true,
                osf::GameplayInventory::backpack_left +
                    (osf::PlayerInventory::grid_width -
                     stock_item.width) *
                        osf::GameplayInventory::cell_size + 1,
                osf::GameplayInventory::backpack_top +
                    (osf::PlayerInventory::grid_height -
                     stock_item.height) *
                        osf::GameplayInventory::cell_size + 1,
            },
            player,
            equipment,
            belt,
            special,
            database,
            99);
    const std::int32_t sale_price =
        osf::itemSalePrice(stock_item, *definition);
    const osf::GameplayVendorResult sold = state.update(
        {false, true, 1, 1},
        vendor,
        inventory,
        player,
        database);
    return check(
        selected_for_sale.pointer_consumed &&
            sold.pointer_consumed &&
            !inventory.holdingItem() &&
            player.gold() ==
                osf::PlayerInventory::maximum_gold_stack -
                    price + sale_price,
        "Selling an owned item did not credit its retail sale price.");
}

}  // namespace

int main() {
    return testVendorStockAndTransactions() ? 0 : 1;
}
