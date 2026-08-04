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

#ifndef SHADOWFLARE_ASSETS_PLAYER_ASSETS_H
#define SHADOWFLARE_ASSETS_PLAYER_ASSETS_H

#include "core/arena.h"
#include "data/caf.h"
#include "data/njp.h"
#include "data/item.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SF_PLAYER_ANIMATION_COUNT 3u
#define SF_PLAYER_DIRECTION_COUNT 8u

typedef struct SfPlayerAssets {
  SfCafSelectedAnimation
    animations[SF_PLAYER_ANIMATION_COUNT][SF_PLAYER_DIRECTION_COUNT];
  SfNjpSparseResource artwork;
  SfNjpSparseResource shadows;
  size_t memory_bytes;
} SfPlayerAssets;

bool sf_player_assets_load(
  SfPlayerAssets *assets, const char *data_root,
  uint8_t gender,
  const uint8_t *appearance_parts, uint8_t appearance_part_count,
  const SfItemReference *visible_items, uint8_t visible_item_count,
  SfArena *arena);
const SfCafSelectedAnimation *sf_player_assets_animation(
  const SfPlayerAssets *assets, uint8_t chart, uint8_t direction);

#endif
