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

#include "data/mct.h"

#include "data/mct_reader.h"
#include "data/mct_sections.h"

#include <stdio.h>
#include <string.h>

#define SF_MCT_FIXED_HEADER_SIZE 0x324L
#define SF_MCT_MAP_PATH_OFFSET 0x114L
#define SF_MCT_MUSIC_OFFSET 0x220L
#define SF_MCT_TITLE_OFFSET 0x224L
static bool sf_mct_read_fixed_string(
    FILE *file, long offset, char *output, size_t capacity) {
  size_t index;
  if (fseek(file, offset, SEEK_SET) != 0 ||
      !sf_mct_reader_read(file, output, capacity)) return false;
  output[capacity - 1u] = '\0';
  for (index = 0u; index < capacity; ++index) {
    if (output[index] == '\0') return true;
  }
  return false;
}

bool sf_mct_load(
    const char *path, SfArena *arena, SfMctScenario *scenario) {
  static const uint8_t expected_header[16] = {
    'M', 'C', 'E', 'D', ' ', 'D', 'A', 'T',
    'A', ' ', 'v', '0', '0', '0', '0', 0x1a
  };
  FILE *file;
  uint8_t header[16];
  size_t mark;
  bool success = false;
  if (!path || !arena || !scenario) return false;
  mark = sf_arena_mark(arena);
  memset(scenario, 0, sizeof(*scenario));
  scenario->music_track = -1;
  file = fopen(path, "rb");
  if (!file) return false;
  if (!sf_mct_reader_read(file, header, sizeof(header)) ||
      memcmp(header, expected_header, sizeof(header)) != 0 ||
      !sf_mct_read_fixed_string(
        file, SF_MCT_MAP_PATH_OFFSET,
        scenario->map_path, sizeof(scenario->map_path)) ||
      fseek(file, SF_MCT_MUSIC_OFFSET, SEEK_SET) != 0 ||
      !sf_mct_reader_i32(file, &scenario->music_track) ||
      !sf_mct_read_fixed_string(
        file, SF_MCT_TITLE_OFFSET,
        scenario->title, sizeof(scenario->title)) ||
      fseek(file, SF_MCT_FIXED_HEADER_SIZE, SEEK_SET) != 0 ||
      !sf_mct_read_sections(file, arena, scenario)) goto done;
  success = true;
done:
  fclose(file);
  if (!success) {
    (void) sf_arena_rewind(arena, mark);
    memset(scenario, 0, sizeof(*scenario));
  }
  return success;
}

const SfMctEntry *sf_mct_find_entry(
    const SfMctScenario *scenario, int32_t key) {
  uint8_t index;
  if (!scenario) return NULL;
  for (index = 0u; index < scenario->entry_count; ++index) {
    if (scenario->entries[index].key == key) return &scenario->entries[index];
  }
  return NULL;
}
