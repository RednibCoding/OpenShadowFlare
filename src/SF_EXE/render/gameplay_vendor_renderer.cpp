#include "gameplay_vendor_renderer.hpp"

#include "gameplay_inventory_renderer.hpp"
#include "gapi/gapi.hpp"
#include "items/vendor_inventory.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "states/gameplay_vendor.hpp"
#include "world/world_scene.hpp"

namespace osf {

void renderGameplayVendor(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const GameplayVendor& vendor,
    const WorldScene& world,
    std::uint32_t gameplay_counter) {
    if (!vendor.active()) {
        return;
    }
    const VendorInventory* inventory =
        world.vendorInventory(vendor.inventoryIndex());
    if (!inventory) {
        return;
    }
    renderer.drawRectangle({0, 0, 320, 412, {0, 0, 0, 255}});
    // FUN_00404760 composes the merchant half from Status.njp 7..9.
    renderer.drawPattern(status_patterns, 7);
    renderer.drawPattern(status_patterns, 8);
    renderer.drawPattern(status_patterns, 9);
    for (const InventoryItem& item : inventory->items()) {
        renderInventoryItem(
            renderer,
            &status_patterns,
            world,
            item,
            GameplayVendor::item_left +
                item.grid_x * GameplayVendor::cell_size,
            GameplayVendor::item_top +
                item.grid_y * GameplayVendor::cell_size,
            gameplay_counter);
    }
}

}  // namespace osf
