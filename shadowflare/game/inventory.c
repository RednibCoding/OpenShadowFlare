/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of OpenShadowFlare.
 *
 * OpenShadowFlare is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * OpenShadowFlare is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with OpenShadowFlare. If not, see <https://www.gnu.org/licenses/>.
 */

#include "game/inventory.h"

#include <string.h>

#define SF_INVENTORY_GOLD_CATEGORY 4u
#define SF_INVENTORY_GOLD_DEFINITION 0
#define SF_INVENTORY_GOLD_STACK 10000

void sf_inventory_init(SfInventoryState *inventory) {
  if (inventory) memset(inventory, 0, sizeof(*inventory));
}

static bool sf_inventory_find_space(
    const SfInventoryState *inventory, uint8_t width, uint8_t height,
    uint8_t *grid_x, uint8_t *grid_y) {
  bool occupied[SF_INVENTORY_WIDTH * SF_INVENTORY_HEIGHT] = {false};
  uint8_t index;
  uint8_t y;
  if (width == 0u || height == 0u || width > SF_INVENTORY_WIDTH ||
      height > SF_INVENTORY_HEIGHT) return false;
  for (index = 0u; index < inventory->count; ++index) {
    const SfInventoryItem *item = &inventory->items[index];
    uint8_t item_y;
    for (item_y = 0u; item_y < item->height; ++item_y) {
      uint8_t item_x;
      for (item_x = 0u; item_x < item->width; ++item_x) {
        const uint8_t x = (uint8_t) (item->grid_x + item_x);
        const uint8_t row = (uint8_t) (item->grid_y + item_y);
        if (x < SF_INVENTORY_WIDTH && row < SF_INVENTORY_HEIGHT)
          occupied[row * SF_INVENTORY_WIDTH + x] = true;
      }
    }
  }
  for (y = 0u; y + height <= SF_INVENTORY_HEIGHT; ++y) {
    uint8_t x;
    for (x = 0u; x + width <= SF_INVENTORY_WIDTH; ++x) {
      uint8_t item_y;
      bool available = true;
      for (item_y = 0u; item_y < height && available; ++item_y) {
        uint8_t item_x;
        for (item_x = 0u; item_x < width; ++item_x) {
          if (occupied[(y + item_y) * SF_INVENTORY_WIDTH + x + item_x]) {
            available = false;
            break;
          }
        }
      }
      if (available) {
        *grid_x = x;
        *grid_y = y;
        return true;
      }
    }
  }
  return false;
}

static bool sf_inventory_item_valid(const SfInventoryItem *item) {
  return item && item->quantity > 0 && item->width > 0u &&
    item->height > 0u && item->width <= SF_INVENTORY_WIDTH &&
    item->height <= SF_INVENTORY_HEIGHT;
}

static bool sf_inventory_item_fits(
    const SfInventoryItem *item, int32_t grid_x, int32_t grid_y) {
  return sf_inventory_item_valid(item) && grid_x >= 0 && grid_y >= 0 &&
    grid_x + item->width <= (int32_t) SF_INVENTORY_WIDTH &&
    grid_y + item->height <= (int32_t) SF_INVENTORY_HEIGHT;
}

static bool sf_inventory_items_overlap(
    const SfInventoryItem *first, const SfInventoryItem *second) {
  return first->grid_x < second->grid_x + second->width &&
    first->grid_x + first->width > second->grid_x &&
    first->grid_y < second->grid_y + second->height &&
    first->grid_y + first->height > second->grid_y;
}

static bool sf_inventory_is_gold(const SfInventoryItem *item) {
  return item->category == SF_INVENTORY_GOLD_CATEGORY &&
    item->definition_id == SF_INVENTORY_GOLD_DEFINITION;
}

int8_t sf_inventory_item_at(
    const SfInventoryState *inventory, uint8_t grid_x, uint8_t grid_y) {
  uint8_t index;
  if (!inventory || grid_x >= SF_INVENTORY_WIDTH ||
      grid_y >= SF_INVENTORY_HEIGHT) return -1;
  for (index = 0u; index < inventory->count; ++index) {
    const SfInventoryItem *item = &inventory->items[index];
    if (grid_x >= item->grid_x && grid_x < item->grid_x + item->width &&
        grid_y >= item->grid_y && grid_y < item->grid_y + item->height)
      return (int8_t) index;
  }
  return -1;
}

bool sf_inventory_take(
    SfInventoryState *inventory, uint8_t index, SfInventoryItem *item) {
  if (!inventory || !item || index >= inventory->count) return false;
  *item = inventory->items[index];
  if (index + 1u < inventory->count)
    memmove(
      &inventory->items[index], &inventory->items[index + 1u],
      (size_t) (inventory->count - index - 1u) * sizeof(inventory->items[0]));
  --inventory->count;
  return true;
}

SfInventoryPlacement sf_inventory_place(
    SfInventoryState *inventory, SfInventoryItem item,
    int32_t grid_x, int32_t grid_y) {
  SfInventoryPlacement result;
  int8_t overlap = -1;
  uint8_t overlap_count = 0u;
  uint8_t index;
  memset(&result, 0, sizeof(result));
  if (!inventory || !sf_inventory_item_fits(&item, grid_x, grid_y))
    return result;
  item.grid_x = (uint8_t) grid_x;
  item.grid_y = (uint8_t) grid_y;
  for (index = 0u; index < inventory->count; ++index) {
    if (!sf_inventory_items_overlap(&item, &inventory->items[index]))
      continue;
    overlap = (int8_t) index;
    if (++overlap_count > 1u) return result;
  }
  if (overlap < 0) {
    if (inventory->count >= SF_INVENTORY_ITEM_LIMIT) return result;
    inventory->items[inventory->count++] = item;
    result.accepted = true;
    return result;
  }
  if (sf_inventory_is_gold(&item) &&
      sf_inventory_is_gold(&inventory->items[(uint8_t) overlap])) {
    SfInventoryItem *stack = &inventory->items[(uint8_t) overlap];
    int32_t moved = SF_INVENTORY_GOLD_STACK - stack->quantity;
    if (moved <= 0) return result;
    if (moved > item.quantity) moved = item.quantity;
    stack->quantity += moved;
    item.quantity -= moved;
    result.accepted = true;
    if (item.quantity > 0) {
      result.held_item = item;
      result.holding_item = true;
    }
    return result;
  }
  result.held_item = inventory->items[(uint8_t) overlap];
  result.holding_item = true;
  if ((uint8_t) overlap + 1u < inventory->count)
    memmove(
      &inventory->items[(uint8_t) overlap],
      &inventory->items[(uint8_t) overlap + 1u],
      (size_t) (inventory->count - (uint8_t) overlap - 1u) *
        sizeof(inventory->items[0]));
  inventory->items[inventory->count - 1u] = item;
  result.accepted = true;
  return result;
}

bool sf_inventory_store_item(
    SfInventoryState *inventory, SfInventoryItem item) {
  SfInventoryState updated;
  if (!inventory || !sf_inventory_item_valid(&item)) return false;
  updated = *inventory;
  if (sf_inventory_is_gold(&item)) {
    uint8_t index;
    for (index = 0u; index < updated.count && item.quantity > 0; ++index) {
      SfInventoryItem *stack = &updated.items[index];
      int32_t moved;
      if (!sf_inventory_is_gold(stack) ||
          stack->quantity >= SF_INVENTORY_GOLD_STACK) continue;
      moved = SF_INVENTORY_GOLD_STACK - stack->quantity;
      if (moved > item.quantity) moved = item.quantity;
      stack->quantity += moved;
      item.quantity -= moved;
    }
  } else if (item.quantity != 1) {
    return false;
  }
  while (item.quantity > 0) {
    SfInventoryItem stored = item;
    uint8_t grid_x;
    uint8_t grid_y;
    if (updated.count >= SF_INVENTORY_ITEM_LIMIT ||
        !sf_inventory_find_space(
          &updated, item.width, item.height, &grid_x, &grid_y)) return false;
    if (sf_inventory_is_gold(&stored) &&
        stored.quantity > SF_INVENTORY_GOLD_STACK)
      stored.quantity = SF_INVENTORY_GOLD_STACK;
    stored.grid_x = grid_x;
    stored.grid_y = grid_y;
    updated.items[updated.count++] = stored;
    item.quantity -= stored.quantity;
  }
  *inventory = updated;
  return true;
}

bool sf_inventory_store(
    SfInventoryState *inventory, const SfItemGroundDefinition *definition,
    int32_t quantity) {
  SfInventoryItem item;
  if (!inventory || !definition || quantity <= 0 ||
      definition->inventory_width <= 0 ||
      definition->inventory_width > (int32_t) SF_INVENTORY_WIDTH ||
      definition->inventory_height <= 0 ||
      definition->inventory_height > (int32_t) SF_INVENTORY_HEIGHT)
    return false;
  item = (SfInventoryItem) {
    definition->definition_id, quantity, definition->maximum_durability,
    definition->category, 0u, 0u,
    (uint8_t) definition->inventory_width,
    (uint8_t) definition->inventory_height,
    definition->variant != 1 && definition->variant != 2};
  return sf_inventory_store_item(inventory, item);
}

int32_t sf_inventory_gold(const SfInventoryState *inventory) {
  int32_t total = 0;
  uint8_t index;
  if (!inventory) return 0;
  for (index = 0u; index < inventory->count; ++index) {
    const SfInventoryItem *item = &inventory->items[index];
    if (item->category != SF_INVENTORY_GOLD_CATEGORY ||
        item->definition_id != SF_INVENTORY_GOLD_DEFINITION) continue;
    if (item->quantity > INT32_MAX - total) return INT32_MAX;
    total += item->quantity;
  }
  return total;
}
