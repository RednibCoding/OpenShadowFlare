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

#include "game/special_items.h"

#include <string.h>

void sf_special_items_init(SfSpecialItemState *items) {
  if (items) memset(items, 0, sizeof(*items));
}

static bool sf_special_item_fits(
    const SfInventoryItem *item, int32_t x, int32_t y) {
  return item && item->quantity > 0 && item->width > 0u &&
    item->height > 0u && x >= 0 && y >= 0 &&
    x + item->width <= (int32_t) SF_SPECIAL_ITEM_WIDTH &&
    y + item->height <= (int32_t) SF_SPECIAL_ITEM_HEIGHT;
}

static bool sf_special_item_overlap(
    const SfInventoryItem *first, const SfInventoryItem *second) {
  return first->grid_x < second->grid_x + second->width &&
    first->grid_x + first->width > second->grid_x &&
    first->grid_y < second->grid_y + second->height &&
    first->grid_y + first->height > second->grid_y;
}

int8_t sf_special_items_item_at(
    const SfSpecialItemState *items, uint8_t grid_x, uint8_t grid_y) {
  uint8_t index;
  if (!items || grid_x >= SF_SPECIAL_ITEM_WIDTH ||
      grid_y >= SF_SPECIAL_ITEM_HEIGHT) return -1;
  for (index = 0u; index < items->count; ++index) {
    const SfInventoryItem *item = &items->items[index];
    if (grid_x >= item->grid_x && grid_x < item->grid_x + item->width &&
        grid_y >= item->grid_y && grid_y < item->grid_y + item->height)
      return (int8_t) index;
  }
  return -1;
}

bool sf_special_items_take(
    SfSpecialItemState *items, uint8_t index, SfInventoryItem *item) {
  if (!items || !item || index >= items->count) return false;
  *item = items->items[index];
  if (index + 1u < items->count)
    memmove(
      &items->items[index], &items->items[index + 1u],
      (size_t) (items->count - index - 1u) * sizeof(items->items[0]));
  --items->count;
  return true;
}

SfInventoryPlacement sf_special_items_place(
    SfSpecialItemState *items, SfInventoryItem item,
    int32_t grid_x, int32_t grid_y) {
  SfInventoryPlacement result;
  int8_t overlap = -1;
  uint8_t overlap_count = 0u;
  uint8_t index;
  memset(&result, 0, sizeof(result));
  if (!items || !sf_special_item_fits(&item, grid_x, grid_y)) return result;
  item.grid_x = (uint8_t) grid_x;
  item.grid_y = (uint8_t) grid_y;
  for (index = 0u; index < items->count; ++index) {
    if (!sf_special_item_overlap(&item, &items->items[index])) continue;
    overlap = (int8_t) index;
    if (++overlap_count > 1u) return result;
  }
  if (overlap < 0) {
    if (items->count >= SF_SPECIAL_ITEM_LIMIT) return result;
    items->items[items->count++] = item;
    result.accepted = true;
    return result;
  }
  if (item.category == 4u && item.definition_id == 0 &&
      items->items[(uint8_t) overlap].category == 4u &&
      items->items[(uint8_t) overlap].definition_id == 0) {
    SfInventoryItem *gold = &items->items[(uint8_t) overlap];
    int32_t moved = 10000 - gold->quantity;
    if (moved <= 0) return result;
    if (moved > item.quantity) moved = item.quantity;
    gold->quantity += moved;
    item.quantity -= moved;
    result.accepted = true;
    if (item.quantity > 0) {
      result.held_item = item;
      result.holding_item = true;
    }
    return result;
  }
  result.held_item = items->items[(uint8_t) overlap];
  result.holding_item = true;
  if ((uint8_t) overlap + 1u < items->count)
    memmove(
      &items->items[(uint8_t) overlap],
      &items->items[(uint8_t) overlap + 1u],
      (size_t) (items->count - (uint8_t) overlap - 1u) *
        sizeof(items->items[0]));
  items->items[items->count - 1u] = item;
  result.accepted = true;
  return result;
}
