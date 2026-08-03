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

#ifndef SHADOWFLARE_DATA_GND_H
#define SHADOWFLARE_DATA_GND_H

#include "core/arena.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_GROUND_EMPTY_PATTERN 0xffu

typedef struct SfGroundCell {
  uint8_t pattern_set;
  uint8_t pattern;
} SfGroundCell;

typedef struct SfGroundMap {
  SfGroundCell *cells;
  uint16_t width;
  uint16_t height;
  uint16_t chip_width;
  uint16_t chip_height;
  int32_t base_magnification_x;
  int32_t base_magnification_y;
} SfGroundMap;

bool sf_gnd_load_render_map(
  const char *path, SfArena *arena, SfGroundMap *map);
const SfGroundCell *sf_ground_cell(
  const SfGroundMap *map, int32_t x, int32_t y);

#endif
