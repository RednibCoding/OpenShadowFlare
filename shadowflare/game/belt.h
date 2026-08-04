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

#ifndef SHADOWFLARE_GAME_BELT_H
#define SHADOWFLARE_GAME_BELT_H

#include "game/inventory.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_BELT_WIDTH 4u
#define SF_BELT_HEIGHT 2u
#define SF_BELT_ITEM_LIMIT 8u

typedef struct SfBeltState {
  SfInventoryItem items[SF_BELT_ITEM_LIMIT];
  uint8_t count;
} SfBeltState;

void sf_belt_init(SfBeltState *belt);
const SfInventoryItem *sf_belt_item_at(
  const SfBeltState *belt, uint8_t grid_x, uint8_t grid_y);
bool sf_belt_take_at(
  SfBeltState *belt, uint8_t grid_x, uint8_t grid_y,
  SfInventoryItem *item);
SfInventoryPlacement sf_belt_place(
  SfBeltState *belt, SfInventoryItem item,
  int32_t grid_x, int32_t grid_y,
  const SfItemGroundDefinition *definition);

#endif
