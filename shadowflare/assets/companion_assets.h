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

#ifndef SHADOWFLARE_ASSETS_COMPANION_ASSETS_H
#define SHADOWFLARE_ASSETS_COMPANION_ASSETS_H

#include "core/arena.h"
#include "data/caf.h"
#include "data/companion_parameters.h"
#include "data/njp.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SF_COMPANION_ANIMATION_COUNT 3u
#define SF_COMPANION_DIRECTION_COUNT 8u
#define SF_COMPANION_PART_LIMIT 8u

typedef struct SfCompanionAssets {
  SfCafSelectedAnimation animations
    [SF_COMPANION_ANIMATION_COUNT][SF_COMPANION_DIRECTION_COUNT];
  SfNjpSparseResource artwork;
  SfNjpSparseResource shadows;
  int32_t resource_id;
  size_t memory_bytes;
  uint8_t selected_parts;
} SfCompanionAssets;

bool sf_companion_assets_load(
  SfCompanionAssets *assets, const char *data_root,
  const SfCompanionProfile *profile, SfArena *arena);

#endif
