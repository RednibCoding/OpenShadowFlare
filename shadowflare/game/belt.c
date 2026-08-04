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

#include "game/belt.h"

#include <string.h>

void sf_belt_init(SfBeltState *belt) {
  if (belt) memset(belt, 0, sizeof(*belt));
}

static bool sf_belt_item_valid(const SfInventoryItem *item) {
  return item && item->quantity == 1 && item->width > 0u &&
    item->height > 0u;
}

static bool sf_belt_contains(
    const SfInventoryItem *item, uint8_t grid_x, uint8_t grid_y) {
  return grid_x >= item->grid_x && grid_x < item->grid_x + item->width &&
    grid_y >= item->grid_y && grid_y < item->grid_y + item->height;
}

const SfInventoryItem *sf_belt_item_at(
    const SfBeltState *belt, uint8_t grid_x, uint8_t grid_y) {
  uint8_t index;
  if (!belt || grid_x >= SF_BELT_WIDTH || grid_y >= SF_BELT_HEIGHT)
    return NULL;
  for (index = 0u; index < belt->count; ++index)
    if (sf_belt_contains(&belt->items[index], grid_x, grid_y))
      return &belt->items[index];
  return NULL;
}

bool sf_belt_take_at(
    SfBeltState *belt, uint8_t grid_x, uint8_t grid_y,
    SfInventoryItem *item) {
  uint8_t index;
  if (!belt || !item || grid_x >= SF_BELT_WIDTH ||
      grid_y >= SF_BELT_HEIGHT) return false;
  for (index = 0u; index < belt->count; ++index) {
    if (!sf_belt_contains(&belt->items[index], grid_x, grid_y)) continue;
    *item = belt->items[index];
    if (index + 1u < belt->count)
      memmove(
        &belt->items[index], &belt->items[index + 1u],
        (size_t) (belt->count - index - 1u) * sizeof(belt->items[0]));
    --belt->count;
    return true;
  }
  return false;
}

static bool sf_belt_overlap(
    const SfInventoryItem *first, const SfInventoryItem *second) {
  return first->grid_x < second->grid_x + second->width &&
    second->grid_x < first->grid_x + first->width &&
    first->grid_y < second->grid_y + second->height &&
    second->grid_y < first->grid_y + first->height;
}

SfInventoryPlacement sf_belt_place(
    SfBeltState *belt, SfInventoryItem item,
    int32_t grid_x, int32_t grid_y,
    const SfItemGroundDefinition *definition) {
  SfInventoryPlacement result;
  int8_t overlap = -1;
  uint8_t index;
  memset(&result, 0, sizeof(result));
  if (!belt || !definition || !sf_belt_item_valid(&item) ||
      item.category != 3u || definition->category != item.category ||
      definition->definition_id != item.definition_id ||
      definition->inventory_width != item.width ||
      definition->inventory_height != item.height ||
      grid_x < 0 || grid_y < 0 ||
      grid_x + (int32_t) item.width > (int32_t) SF_BELT_WIDTH ||
      grid_y + (int32_t) item.height > (int32_t) SF_BELT_HEIGHT) return result;
  item.grid_x = (uint8_t) grid_x;
  item.grid_y = (uint8_t) grid_y;
  for (index = 0u; index < belt->count; ++index) {
    if (!sf_belt_overlap(&item, &belt->items[index])) continue;
    if (overlap >= 0) return result;
    overlap = (int8_t) index;
  }
  if (overlap < 0) {
    if (belt->count >= SF_BELT_ITEM_LIMIT) return result;
    belt->items[belt->count++] = item;
  } else {
    result.held_item = belt->items[(uint8_t) overlap];
    result.holding_item = true;
    belt->items[(uint8_t) overlap] = item;
  }
  result.accepted = true;
  return result;
}
