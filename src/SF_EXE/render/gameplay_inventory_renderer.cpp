#include "gameplay_inventory_renderer.hpp"

#include "gapi/gapi.hpp"
#include "items/item_condition.hpp"
#include "items/item_database.hpp"
#include "items/player_equipment.hpp"
#include "items/player_inventory.hpp"
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

void drawPanelText(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    const std::string& text,
    std::int32_t x,
    std::int32_t y) {
    renderer.drawText(
        font,
        text,
        {x + 1, y + 1, {0, 0, 0, 255}, 1000, 2});
    renderer.drawText(
        font,
        text,
        {x, y, kValueColor, 1000, 2});
}

void drawRightAlignedNumber(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    std::int32_t value,
    std::int32_t right,
    std::int32_t y) {
    const std::string text =
        std::to_string(value < 0 ? 0 : value);
    drawPanelText(
        renderer,
        font,
        text,
        right -
            static_cast<std::int32_t>(text.size()) * 8,
        y);
}

bool drawInventoryItem(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
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
        status_patterns.patterns().size() > 16) {
        renderer.drawPattern(
            status_patterns,
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

}  // namespace

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
        world.playerData().gender() == 1 ? 1 : 0);

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
    drawRightAlignedNumber(
        renderer,
        font,
        world.playerEquipment().totalWeight(
            world.itemDatabase()),
        471,
        224);

    for (std::size_t index = 0;
         index < PlayerEquipment::slot_count;
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
        drawInventoryItem(
            renderer,
            status_patterns,
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
        drawInventoryItem(
            renderer,
            status_patterns,
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
    drawInventoryItem(
        renderer,
        status_patterns,
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
