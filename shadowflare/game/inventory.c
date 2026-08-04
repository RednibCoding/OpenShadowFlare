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

bool sf_inventory_store(
    SfInventoryState *inventory, const SfItemGroundDefinition *definition,
    int32_t quantity) {
  SfInventoryState updated;
  int32_t remaining;
  uint8_t width;
  uint8_t height;
  if (!inventory || !definition || quantity <= 0 ||
      definition->inventory_width <= 0 ||
      definition->inventory_width > (int32_t) SF_INVENTORY_WIDTH ||
      definition->inventory_height <= 0 ||
      definition->inventory_height > (int32_t) SF_INVENTORY_HEIGHT)
    return false;
  updated = *inventory;
  remaining = quantity;
  width = (uint8_t) definition->inventory_width;
  height = (uint8_t) definition->inventory_height;
  if (definition->category == SF_INVENTORY_GOLD_CATEGORY &&
      definition->definition_id == SF_INVENTORY_GOLD_DEFINITION) {
    uint8_t index;
    for (index = 0u; index < updated.count && remaining > 0; ++index) {
      SfInventoryItem *item = &updated.items[index];
      int32_t moved;
      if (item->category != definition->category ||
          item->definition_id != definition->definition_id ||
          item->quantity >= SF_INVENTORY_GOLD_STACK) continue;
      moved = SF_INVENTORY_GOLD_STACK - item->quantity;
      if (moved > remaining) moved = remaining;
      item->quantity += moved;
      remaining -= moved;
    }
  } else if (quantity != 1) {
    return false;
  }
  while (remaining > 0) {
    SfInventoryItem *item;
    uint8_t grid_x;
    uint8_t grid_y;
    int32_t stack = remaining;
    if (updated.count >= SF_INVENTORY_ITEM_LIMIT ||
        !sf_inventory_find_space(
          &updated, width, height, &grid_x, &grid_y)) return false;
    if (definition->category == SF_INVENTORY_GOLD_CATEGORY &&
        definition->definition_id == SF_INVENTORY_GOLD_DEFINITION &&
        stack > SF_INVENTORY_GOLD_STACK) stack = SF_INVENTORY_GOLD_STACK;
    item = &updated.items[updated.count++];
    *item = (SfInventoryItem) {
      definition->definition_id, stack, definition->maximum_durability,
      definition->category, grid_x, grid_y, width, height,
      definition->variant != 1 && definition->variant != 2};
    remaining -= stack;
  }
  *inventory = updated;
  return true;
}
