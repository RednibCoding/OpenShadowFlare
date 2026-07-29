#include "gameplay_inventory_renderer.hpp"

#include "gapi/gapi.hpp"
#include "items/item_database.hpp"
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

}  // namespace

void renderGameplayInventory(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& font,
    const GameplayInventory& inventory,
    const WorldScene& world) {
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
        // Retail weight is the sum of equipped and belt-pocket items, not
        // backpack contents. Those stores are still empty in this slice.
        0,
        471,
        224);

    for (const InventoryItem& item : owned.items()) {
        const ItemDefinition* definition =
            world.itemDatabase().find(
                item.category,
                item.definition_id);
        if (!definition ||
            definition->inventory_pattern < 0) {
            continue;
        }
        const gapi::NjpImage* patterns =
            world.itemInventoryPatterns().group(
                definition->inventory_pattern_group);
        if (!patterns ||
            static_cast<std::size_t>(
                definition->inventory_pattern) >=
                patterns->patterns().size()) {
            continue;
        }
        renderer.drawPattern(
            *patterns,
            static_cast<std::size_t>(
                definition->inventory_pattern),
            {
                GameplayInventory::backpack_left +
                    item.grid_x *
                        GameplayInventory::cell_size,
                GameplayInventory::backpack_top +
                    item.grid_y *
                        GameplayInventory::cell_size,
                1000,
                1000,
                1000,
                1000,
                1000,
                1000,
                1000,
                definition->inventory_palette,
            });
    }

    renderer.drawPattern(
        status_patterns,
        inventory.closeHovered() ? 75 : 74);
}

}  // namespace osf
