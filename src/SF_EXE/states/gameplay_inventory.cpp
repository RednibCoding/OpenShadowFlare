#include "gameplay_inventory.hpp"

#include "items/player_inventory.hpp"

#include <cstddef>

namespace osf {
namespace {

constexpr std::int32_t kCloseLeft = 375;
constexpr std::int32_t kCloseTop = 393;
constexpr std::int32_t kCloseRight = 443;
constexpr std::int32_t kCloseBottom = 404;

bool inside(
    std::int32_t x,
    std::int32_t y,
    std::int32_t left,
    std::int32_t top,
    std::int32_t right,
    std::int32_t bottom) {
    return x >= left && x < right &&
           y >= top && y < bottom;
}

}  // namespace

void GameplayInventory::open() {
    active_ = true;
    close_hovered_ = false;
    hovered_item_index_ = -1;
}

void GameplayInventory::close() {
    active_ = false;
    close_hovered_ = false;
    hovered_item_index_ = -1;
}

GameplayInventoryResult GameplayInventory::update(
    const GameplayInventoryInput& input,
    const PlayerInventory& inventory) {
    GameplayInventoryResult result;
    if (input.toggle_pressed) {
        if (active_) {
            close();
        } else {
            open();
            updateHover(
                input.pointer_x,
                input.pointer_y,
                inventory);
        }
        return result;
    }
    if (!active_) {
        return result;
    }

    updateHover(
        input.pointer_x,
        input.pointer_y,
        inventory);
    if (input.close_pressed ||
        (input.pointer_primary_pressed &&
         close_hovered_)) {
        result.pointer_consumed =
            input.pointer_primary_pressed;
        close();
    } else if (
        input.pointer_primary_pressed &&
        input.pointer_x >= panel_left &&
        input.pointer_y < 412) {
        result.pointer_consumed = true;
    }
    return result;
}

bool GameplayInventory::active() const {
    return active_;
}

bool GameplayInventory::closeHovered() const {
    return close_hovered_;
}

std::int32_t GameplayInventory::hoveredItemIndex() const {
    return hovered_item_index_;
}

void GameplayInventory::updateHover(
    std::int32_t pointer_x,
    std::int32_t pointer_y,
    const PlayerInventory& inventory) {
    close_hovered_ = inside(
        pointer_x,
        pointer_y,
        kCloseLeft,
        kCloseTop,
        kCloseRight,
        kCloseBottom);
    hovered_item_index_ = -1;
    const auto& items = inventory.items();
    for (std::size_t index = 0;
         index < items.size();
         ++index) {
        const InventoryItem& item = items[index];
        if (inside(
                pointer_x,
                pointer_y,
                backpack_left + item.grid_x * cell_size,
                backpack_top + item.grid_y * cell_size,
                backpack_left +
                    (item.grid_x + item.width) * cell_size,
                backpack_top +
                    (item.grid_y + item.height) * cell_size)) {
            hovered_item_index_ =
                static_cast<std::int32_t>(index);
            break;
        }
    }
}

}  // namespace osf
