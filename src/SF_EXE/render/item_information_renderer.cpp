#include "item_information_renderer.hpp"

#include "gapi/gapi.hpp"
#include "items/item_database.hpp"
#include "items/item_information.hpp"
#include "items/player_inventory.hpp"
#include "items/vendor_inventory.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "states/gameplay_inventory.hpp"
#include "states/gameplay_vendor.hpp"
#include "ui/conversation_layout.hpp"
#include "world/world_scene.hpp"

#include <algorithm>
#include <cstdint>
#include <string>

namespace osf {
namespace {

constexpr std::int32_t kScreenWidth = 640;
constexpr std::int32_t kScreenHeight = 480;
constexpr std::int32_t kInformationPadding = 4;
constexpr std::int32_t kBackgroundOpacity = 600;
constexpr std::int32_t kBorderOpacity = 500;

gapi::Color informationColor(std::int32_t variant) {
    switch (variant) {
    case 1:
        return {192, 128, 128, 255};
    case 2:
        return {128, 192, 224, 255};
    case 3:
        return {92, 128, 224, 255};
    default:
        return {224, 224, 224, 255};
    }
}

void renderInformation(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    const InventoryItem& item,
    const ItemDefinition& definition,
    ItemInformationPrice price,
    std::int32_t pointer_x,
    std::int32_t pointer_y) {
    if (font.patterns().empty()) {
        return;
    }
    const std::string text =
        itemInformationText(item, definition, price);
    if (text.empty()) {
        return;
    }
    const gapi::NjpPattern& base_pattern =
        font.patterns().front();
    const std::int32_t cell_width = base_pattern.width / 16;
    const std::int32_t cell_height = base_pattern.height / 16;
    if (cell_width <= 0 || cell_height <= 0) {
        return;
    }
    const std::int32_t width =
        bitmapTextPixelWidth(text, cell_width) +
        kInformationPadding * 2;
    const std::int32_t height =
        bitmapTextLineCount(text) * cell_height +
        kInformationPadding * 2;
    const std::int32_t x = std::clamp(
        pointer_x - width / 2,
        1,
        std::max(1, kScreenWidth - width));
    const std::int32_t y = std::clamp(
        pointer_y + 8,
        1,
        std::max(1, kScreenHeight - height));
    const gapi::Color black{0, 0, 0, 255};
    const gapi::Color white{255, 255, 255, 255};
    renderer.drawRectangle({
        x, y, width, height, black, 1000, kBackgroundOpacity});
    renderer.drawRectangle({
        x - 1, y - 1, width + 1, 1,
        white, 1000, kBorderOpacity});
    renderer.drawRectangle({
        x - 1, y - 1, 1, height + 1,
        white, 1000, kBorderOpacity});
    renderer.drawRectangle({
        x + width - 1, y, 1, height,
        white, 1000, kBorderOpacity});
    renderer.drawRectangle({
        x, y + height - 1, width, 1,
        white, 1000, kBorderOpacity});
    const std::int32_t text_x = x + kInformationPadding;
    const std::int32_t text_y = y + kInformationPadding;
    renderer.drawText(
        font, text, {text_x + 1, text_y + 1, black});
    renderer.drawText(
        font,
        text,
        {text_x, text_y, informationColor(definition.variant)});
}

}  // namespace

void renderItemInformation(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    const GameplayInventory& inventory,
    const WorldScene& world) {
    const InventoryItem* item = inventory.informationItem(
        world.playerInventory(),
        world.playerEquipment(),
        world.playerSpecialItems(),
        &world.playerGiantWarehouse());
    const ItemDefinition* definition = item
        ? world.itemDatabase().find(
              item->category, item->definition_id)
        : nullptr;
    if (item && definition) {
        renderInformation(
            renderer,
            font,
            *item,
            *definition,
            ItemInformationPrice::sale,
            inventory.pointerX(),
            inventory.pointerY());
    }
}

void renderVendorItemInformation(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    const GameplayVendor& vendor,
    const WorldScene& world) {
    const VendorInventory* stock =
        world.vendorInventory(vendor.inventoryIndex());
    const InventoryItem* item = stock
        ? vendor.informationItem(*stock)
        : nullptr;
    const ItemDefinition* definition = item
        ? world.itemDatabase().find(
              item->category, item->definition_id)
        : nullptr;
    if (item && definition) {
        renderInformation(
            renderer,
            font,
            *item,
            *definition,
            ItemInformationPrice::purchase,
            vendor.pointerX(),
            vendor.pointerY());
    }
}

}  // namespace osf
