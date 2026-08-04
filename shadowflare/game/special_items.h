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

#ifndef SHADOWFLARE_GAME_SPECIAL_ITEMS_H
#define SHADOWFLARE_GAME_SPECIAL_ITEMS_H

#include "game/inventory.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_SPECIAL_ITEM_WIDTH 9u
#define SF_SPECIAL_ITEM_HEIGHT 10u
#define SF_SPECIAL_ITEM_LIMIT 90u

typedef struct SfSpecialItemState {
  SfInventoryItem items[SF_SPECIAL_ITEM_LIMIT];
  uint8_t count;
} SfSpecialItemState;

void sf_special_items_init(SfSpecialItemState *items);
int8_t sf_special_items_item_at(
  const SfSpecialItemState *items, uint8_t grid_x, uint8_t grid_y);
bool sf_special_items_take(
  SfSpecialItemState *items, uint8_t index, SfInventoryItem *item);
SfInventoryPlacement sf_special_items_place(
  SfSpecialItemState *items, SfInventoryItem item,
  int32_t grid_x, int32_t grid_y);

#endif
