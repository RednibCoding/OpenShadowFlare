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

#ifndef SHADOWFLARE_DATA_OBL_H
#define SHADOWFLARE_DATA_OBL_H

#include "core/arena.h"
#include "core/bounds.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct SfMapObject {
  int32_t world_x;
  int32_t world_y;
  SfObjectBounds judgement;
  int16_t pattern_set;
  int16_t pattern;
  int16_t palette;
  int16_t opacity;
  int16_t status;
  int16_t height;
  int16_t red_strength;
  int16_t green_strength;
  int16_t blue_strength;
} SfMapObject;

typedef struct SfObjectMap {
  SfMapObject *objects;
  uint16_t count;
  uint8_t version;
} SfObjectMap;

bool sf_obl_load(const char *path, SfArena *arena, SfObjectMap *map);

#endif
