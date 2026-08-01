#include "item_information_renderer.hpp"

#include "ui/conversation_layout.hpp"
#include "gapi/gapi.hpp"
#include "items/item_database.hpp"
#include "items/item_information.hpp"
#include "items/player_inventory.hpp"
#include "libs/RKC_UPDIB/rkc_updib.hpp"
#include "states/gameplay_inventory.hpp"
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

}  // namespace

void renderItemInformation(
    gapi::Backend& renderer,
    const gapi::NjpImage& font,
    const GameplayInventory& inventory,
    const WorldScene& world) {
    const InventoryItem* item =
        inventory.informationItem(
            world.playerInventory(),
            world.playerEquipment(),
            world.playerSpecialItems());
    if (!item || font.patterns().empty()) {
        return;
    }
    const ItemDefinition* definition =
        world.itemDatabase().find(
            item->category,
            item->definition_id);
    if (!definition) {
        return;
    }

    const std::string text =
        itemInformationText(*item, *definition);
    if (text.empty()) {
        return;
    }
    const gapi::NjpPattern& base_pattern =
        font.patterns().front();
    const std::int32_t cell_width =
        base_pattern.width / 16;
    const std::int32_t cell_height =
        base_pattern.height / 16;
    if (cell_width <= 0 || cell_height <= 0) {
        return;
    }

    const std::int32_t text_width =
        bitmapTextPixelWidth(text, cell_width);
    const std::int32_t text_height =
        bitmapTextLineCount(text) * cell_height;
    const std::int32_t width =
        text_width + kInformationPadding * 2;
    const std::int32_t height =
        text_height + kInformationPadding * 2;
    const std::int32_t x = std::clamp(
        inventory.pointerX() - width / 2,
        std::int32_t{1},
        std::max(std::int32_t{1}, kScreenWidth - width));
    const std::int32_t y = std::clamp(
        inventory.pointerY() + 8,
        std::int32_t{1},
        std::max(std::int32_t{1}, kScreenHeight - height));
    const gapi::Color black{0, 0, 0, 255};
    const gapi::Color white{255, 255, 255, 255};

    renderer.drawRectangle({
        x,
        y,
        width,
        height,
        black,
        1000,
        kBackgroundOpacity,
    });
    renderer.drawRectangle({
        x - 1,
        y - 1,
        width + 1,
        1,
        white,
        1000,
        kBorderOpacity,
    });
    renderer.drawRectangle({
        x - 1,
        y - 1,
        1,
        height + 1,
        white,
        1000,
        kBorderOpacity,
    });
    renderer.drawRectangle({
        x + width - 1,
        y,
        1,
        height,
        white,
        1000,
        kBorderOpacity,
    });
    renderer.drawRectangle({
        x,
        y + height - 1,
        width,
        1,
        white,
        1000,
        kBorderOpacity,
    });

    const std::int32_t text_x =
        x + kInformationPadding;
    const std::int32_t text_y =
        y + kInformationPadding;

    renderer.drawText(
        font,
        text,
        {text_x + 1, text_y + 1, black});
    renderer.drawText(
        font,
        text,
        {
            text_x,
            text_y,
            informationColor(definition->variant),
        });
}

}  // namespace osf
