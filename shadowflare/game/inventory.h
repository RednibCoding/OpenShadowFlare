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

#ifndef SHADOWFLARE_GAME_INVENTORY_H
#define SHADOWFLARE_GAME_INVENTORY_H

#include "data/item.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_INVENTORY_WIDTH 9u
#define SF_INVENTORY_HEIGHT 4u
#define SF_INVENTORY_ITEM_LIMIT 36u

typedef struct SfInventoryItem {
  int32_t definition_id;
  int32_t quantity;
  int32_t durability;
  uint8_t category;
  uint8_t grid_x;
  uint8_t grid_y;
  uint8_t width;
  uint8_t height;
  bool identified;
} SfInventoryItem;

typedef struct SfInventoryState {
  SfInventoryItem items[SF_INVENTORY_ITEM_LIMIT];
  uint8_t count;
} SfInventoryState;

typedef struct SfInventoryPlacement {
  SfInventoryItem held_item;
  bool accepted;
  bool holding_item;
} SfInventoryPlacement;

typedef struct SfInventoryTransferState {
  SfInventoryItem held_item;
  bool holding_item;
} SfInventoryTransferState;

void sf_inventory_init(SfInventoryState *inventory);
bool sf_inventory_store(
  SfInventoryState *inventory, const SfItemGroundDefinition *definition,
  int32_t quantity);
bool sf_inventory_store_item(
  SfInventoryState *inventory, SfInventoryItem item);
int8_t sf_inventory_item_at(
  const SfInventoryState *inventory, uint8_t grid_x, uint8_t grid_y);
bool sf_inventory_take(
  SfInventoryState *inventory, uint8_t index, SfInventoryItem *item);
SfInventoryPlacement sf_inventory_place(
  SfInventoryState *inventory, SfInventoryItem item,
  int32_t grid_x, int32_t grid_y);
int32_t sf_inventory_gold(const SfInventoryState *inventory);

#endif
