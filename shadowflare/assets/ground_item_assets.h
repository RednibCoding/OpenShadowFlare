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

#ifndef SHADOWFLARE_ASSETS_GROUND_ITEM_ASSETS_H
#define SHADOWFLARE_ASSETS_GROUND_ITEM_ASSETS_H

#include "core/arena.h"
#include "data/caf.h"
#include "data/item.h"
#include "data/njp.h"
#include "data/scs.h"
#include "data/voc.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SF_GROUND_ITEM_DEFINITION_LIMIT 16u
#define SF_GROUND_ITEM_RESOURCE_LIMIT 8u

typedef struct SfGroundItemVisual {
  SfCafSelectedAnimation *animations;
  uint16_t *charts;
  SfNjpSparseResource artwork;
  SfNjpSparseResource shadows;
  int32_t resource_id;
  uint8_t animation_count;
} SfGroundItemVisual;

typedef struct SfGroundItemAssets {
  SfItemGroundDefinition *definitions;
  SfGroundItemVisual *visuals;
  SfPcmU8 sounds[7];
  size_t memory_bytes;
  uint8_t definition_count;
  uint8_t visual_count;
} SfGroundItemAssets;

bool sf_ground_item_assets_load(
  SfGroundItemAssets *assets, const char *data_root,
  const SfScsScript *script,
  const SfItemReference *retained_items, uint8_t retained_item_count,
  SfArena *arena);
const SfGroundItemVisual *sf_ground_item_visual(
  const SfGroundItemAssets *assets, int32_t resource_id);
const SfCafSelectedAnimation *sf_ground_item_animation(
  const SfGroundItemVisual *visual, int32_t chart);
const SfPcmU8 *sf_ground_item_sound(
  const SfGroundItemAssets *assets, uint16_t sample);

#endif
