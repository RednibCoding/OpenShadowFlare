#include "gameplay_inventory_renderer.hpp"

#include "gapi/gapi.hpp"
#include "items/item_condition.hpp"
#include "items/item_database.hpp"
#include "items/player_belt.hpp"
#include "items/player_equipment.hpp"
#include "items/player_giant_warehouse.hpp"
#include "items/player_inventory.hpp"
#include "items/player_special_items.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "resources/item_inventory_resource.hpp"
#include "states/gameplay_inventory.hpp"
#include "world/world_scene.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace osf {
namespace {

constexpr gapi::Color kValueColor{224, 192, 128, 255};
constexpr gapi::Color kNormalValueColor{224, 224, 224, 255};
constexpr gapi::Color kLowerValueColor{224, 64, 64, 255};

void drawPanelText(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    const std::string& text,
    std::int32_t x,
    std::int32_t y,
    gapi::Color color = kValueColor) {
    renderer.drawText(
        font,
        text,
        {x + 1, y + 1, {0, 0, 0, 255}, 1000, 2});
    renderer.drawText(
        font,
        text,
        {x, y, color, 1000, 2});
}

void drawRightAlignedNumber(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    std::int32_t value,
    std::int32_t right,
    std::int32_t y,
    gapi::Color color = kValueColor) {
    const std::string text =
        std::to_string(value < 0 ? 0 : value);
    drawPanelText(
        renderer,
        font,
        text,
        right -
            static_cast<std::int32_t>(text.size()) * 8,
        y,
        color);
}

}  // namespace

bool renderInventoryItem(
    gapi::Backend& renderer,
    const gapi::NjpImage* status_patterns,
    const WorldScene& world,
    const InventoryItem& item,
    std::int32_t x,
    std::int32_t y,
    std::uint32_t gameplay_counter) {
    const ItemDefinition* definition =
        world.itemDatabase().find(
            item.category,
            item.definition_id);
    if (!definition ||
        definition->inventory_pattern < 0) {
        return false;
    }
    const gapi::NjpImage* patterns =
        world.itemInventoryPatterns().group(
            definition->inventory_pattern_group);
    if (!patterns ||
        static_cast<std::size_t>(
            definition->inventory_pattern) >=
            patterns->patterns().size()) {
        return false;
    }
    if (!renderer.drawPattern(
        *patterns,
        static_cast<std::size_t>(
            definition->inventory_pattern),
        {
            x,
            y,
            1000,
            1000,
            1000,
            1000,
            1000,
            1000,
            1000,
            definition->inventory_palette,
        })) {
        return false;
    }

    if (itemConditionWarningVisible(
            item, *definition, gameplay_counter) &&
        status_patterns &&
        status_patterns->patterns().size() > 16) {
        renderer.drawPattern(
            *status_patterns,
            16,
            {
                x + item.width *
                        GameplayInventory::cell_size -
                    16,
                y + item.height *
                        GameplayInventory::cell_size -
                    16,
            });
    }
    return true;
}

void renderGameplayInventory(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& font,
    const GameplayInventory& inventory,
    const WorldScene& world,
    std::uint32_t gameplay_counter) {
    if (!inventory.active() || !world.hasPlayer()) {
        return;
    }

    // The retail world viewport ends at x=320 while this panel is open.
    // Clear that half before drawing the cut-out Status.njp layers so scenery
    // cannot show through their transparent pixels.
    renderer.drawRectangle({
        GameplayInventory::panel_left,
        0,
        320,
        412,
        {0, 0, 0, 255},
    });

    // FUN_00404760 composes the inventory from these authored Status.njp
    // layers. Patterns 0 and 1 are the male/female equipment silhouettes.
    renderer.drawPattern(status_patterns, 2);
    renderer.drawPattern(status_patterns, 3);
    renderer.drawPattern(
        status_patterns,
        world.playerData().gender() == 1 ? 0 : 1);

    const PlayerInventory& owned = world.playerInventory();
    drawPanelText(
        renderer,
        font,
        "Total Gold",
        342,
        30);
    drawRightAlignedNumber(
        renderer,
        font,
        owned.gold(),
        471,
        28);

    // InventoryStatusDisplay (0x00408a80) renders mines as a separate
    // resource. The icon is present only for a nonzero count, while the
    // current count, slash, and equipment-derived maximum remain visible.
    const std::int32_t mine_count = world.playerMineCount();
    const std::int32_t maximum_mines =
        world.playerMaximumMineCount();
    if (mine_count != 0) {
        renderer.drawPattern(status_patterns, 67);
    }
    const gapi::Color mine_value_color =
        maximum_mines < 10
            ? kLowerValueColor
            : (maximum_mines > 10
                   ? kValueColor
                   : kNormalValueColor);
    drawRightAlignedNumber(
        renderer,
        font,
        mine_count,
        446,
        118,
        mine_value_color);
    drawPanelText(
        renderer,
        font,
        "/",
        445,
        117,
        kNormalValueColor);
    drawRightAlignedNumber(
        renderer,
        font,
        maximum_mines,
        471,
        118,
        mine_value_color);
    drawRightAlignedNumber(
        renderer,
        font,
        world.playerEquipment().totalWeight(
            world.itemDatabase()),
        471,
        224);

    for (std::size_t index = 0;
         index < PlayerEquipment::visible_slot_count;
         ++index) {
        const EquipmentSlot slot =
            static_cast<EquipmentSlot>(index);
        const InventoryItem* equipped =
            world.playerEquipment().item(slot);
        if (!equipped) {
            continue;
        }
        // FUN_00407170 centers the complete inventory footprint within each
        // authored equipment region.
        const EquipmentRegion region =
            GameplayInventory::equipmentRegion(slot);
        renderInventoryItem(
            renderer,
            &status_patterns,
            world,
            *equipped,
            region.left +
                (region.width_in_cells - equipped->width) *
                    GameplayInventory::cell_size / 2,
            region.top +
                (region.height_in_cells - equipped->height) *
                    GameplayInventory::cell_size / 2,
            gameplay_counter);
    }

    const auto& items = owned.items();
    for (std::size_t index = 0;
         index < items.size();
         ++index) {
        const InventoryItem& item = items[index];
        renderInventoryItem(
            renderer,
            &status_patterns,
            world,
            item,
            GameplayInventory::backpack_left +
                item.grid_x *
                    GameplayInventory::cell_size,
            GameplayInventory::backpack_top +
                item.grid_y *
                    GameplayInventory::cell_size,
            gameplay_counter);
    }

    renderer.drawPattern(
        status_patterns,
        inventory.closeHovered() ? 75 : 74);
}

void renderGameplayBeltItems(
    gapi::Backend& renderer,
    const WorldScene& world) {
    for (const InventoryItem& item :
         world.playerBelt().items()) {
        // FUN_00407170 uses staggered origins for the two 4-cell rows.
        renderInventoryItem(
            renderer,
            nullptr,
            world,
            item,
            (item.grid_y == 0 ? 357 : 405) +
                item.grid_x *
                    GameplayInventory::cell_size,
            413 +
                item.grid_y *
                    GameplayInventory::cell_size,
            0);
    }
}

void renderGameplaySpecialItems(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const GameplayInventory& inventory,
    const WorldScene& world,
    std::uint32_t gameplay_counter) {
    if (!inventory.leftStorageActive() ||
        !world.hasPlayer()) {
        return;
    }

    renderer.drawRectangle({
        0,
        0,
        GameplayInventory::panel_left,
        412,
        {0, 0, 0, 255},
    });
    renderer.drawPattern(status_patterns, 14);
    renderer.drawPattern(status_patterns, 15);

    const PlayerSpecialItems* storage =
        &world.playerSpecialItems();
    if (inventory.giantWarehouseActive()) {
        const PlayerGiantWarehouse& giant =
            world.playerGiantWarehouse();
        renderer.drawPattern(status_patterns, 73);
        for (std::size_t page = 0;
             page < PlayerGiantWarehouse::page_count;
             ++page) {
            std::size_t pattern = 74;
            if (giant.pageEnabled(page)) {
                pattern = page == giant.selectedPage()
                    ? 85 + page
                    : 75 + page;
            }
            renderer.drawPattern(
                status_patterns,
                pattern,
                {
                    24 + static_cast<std::int32_t>(page) * 24,
                    41,
                });
        }
        storage = &giant.page(giant.selectedPage());
    }

    for (const InventoryItem& item :
         storage->items()) {
        renderInventoryItem(
            renderer,
            &status_patterns,
            world,
            item,
            GameplayInventory::special_left +
                item.grid_x *
                    GameplayInventory::cell_size,
            GameplayInventory::special_top +
                item.grid_y *
                    GameplayInventory::cell_size,
            gameplay_counter);
    }
}

void renderHeldInventoryItem(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const GameplayInventory& inventory,
    const WorldScene& world,
    std::uint32_t gameplay_counter) {
    const InventoryItem* item =
        inventory.heldItem();
    if (!item) {
        return;
    }
    renderInventoryItem(
        renderer,
        &status_patterns,
        world,
        *item,
        inventory.pointerX() -
            item->width *
                GameplayInventory::cell_size / 2,
        inventory.pointerY() -
            item->height *
                GameplayInventory::cell_size / 2,
        gameplay_counter);
}

}  // namespace osf
