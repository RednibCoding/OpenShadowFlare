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

#ifndef SHADOWFLARE_DATA_MCT_H
#define SHADOWFLARE_DATA_MCT_H

#include <stdbool.h>
#include <stdint.h>

#define SF_MCT_ENTRY_LIMIT 64u

typedef struct SfMctEntry {
  int32_t key;
  int32_t world_x;
  int32_t world_y;
  int32_t direction;
} SfMctEntry;

typedef struct SfMctScenario {
  char map_path[260];
  char title[256];
  SfMctEntry entries[SF_MCT_ENTRY_LIMIT];
  int32_t music_track;
  uint8_t entry_count;
} SfMctScenario;

bool sf_mct_load(const char *path, SfMctScenario *scenario);
const SfMctEntry *sf_mct_find_entry(
  const SfMctScenario *scenario, int32_t key);

#endif
