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

#ifndef SHADOWFLARE_ASSETS_INVENTORY_ITEM_ASSETS_H
#define SHADOWFLARE_ASSETS_INVENTORY_ITEM_ASSETS_H

#include "core/arena.h"
#include "data/item.h"
#include "data/njp.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_INVENTORY_ARTWORK_GROUP_LIMIT 14u

typedef struct SfInventoryArtworkGroup {
  SfNjpSparseResource resource;
  uint8_t source_group;
} SfInventoryArtworkGroup;

typedef struct SfInventoryItemAssets {
  SfInventoryArtworkGroup groups[SF_INVENTORY_ARTWORK_GROUP_LIMIT];
  uint8_t group_count;
} SfInventoryItemAssets;

bool sf_inventory_item_assets_load(
  SfInventoryItemAssets *assets, const char *data_root,
  const SfItemGroundDefinition *definitions, uint8_t definition_count,
  SfArena *arena);
const SfNjpSparseResource *sf_inventory_item_artwork(
  const SfInventoryItemAssets *assets, int32_t source_group);

#endif
