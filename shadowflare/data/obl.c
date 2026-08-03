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

#include "data/obl.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

static bool sf_obl_read(FILE *file, void *output, size_t size) {
  return size == 0u || fread(output, 1u, size, file) == size;
}

static bool sf_obl_i16(FILE *file, int16_t *value) {
  uint8_t bytes[2];
  if (!sf_obl_read(file, bytes, sizeof(bytes))) return false;
  *value = (int16_t) ((uint16_t) bytes[0] | ((uint16_t) bytes[1] << 8u));
  return true;
}

static bool sf_obl_i32(FILE *file, int32_t *value) {
  uint8_t bytes[4];
  uint32_t raw;
  if (!sf_obl_read(file, bytes, sizeof(bytes))) return false;
  raw = (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8u) |
    ((uint32_t) bytes[2] << 16u) | ((uint32_t) bytes[3] << 24u);
  *value = (int32_t) raw;
  return true;
}

bool sf_obl_load(const char *path, SfArena *arena, SfObjectMap *map) {
  FILE *file;
  char header[16];
  int32_t count;
  int version;
  int32_t index;
  size_t mark;
  bool success = false;
  if (!path || !arena || !map) return false;
  mark = sf_arena_mark(arena);
  memset(map, 0, sizeof(*map));
  file = fopen(path, "rb");
  if (!file) return false;
  if (!sf_obl_read(file, header, sizeof(header)) ||
      memcmp(header, "RPGSCRN_OBJv", 12u) != 0 ||
      header[12] < '0' || header[12] > '9' ||
      header[13] < '0' || header[13] > '9' ||
      header[14] < '0' || header[14] > '9' ||
      !sf_obl_i32(file, &count) || count < 0 || count > UINT16_MAX)
    goto done;
  version = (header[12] - '0') * 100 +
    (header[13] - '0') * 10 + header[14] - '0';
  if (version < 0 || version > 1) goto done;
  map->objects = (SfMapObject *) sf_arena_push_zero(
    arena, (size_t) count * sizeof(*map->objects), sizeof(void *));
  if (count > 0 && !map->objects) goto done;
  for (index = 0; index < count; ++index) {
    SfMapObject *object = &map->objects[index];
    object->red_strength = 1000;
    object->green_strength = 1000;
    object->blue_strength = 1000;
    if (!sf_obl_i32(file, &object->world_x) ||
        !sf_obl_i32(file, &object->world_y) ||
        !sf_obl_i16(file, &object->pattern_set) ||
        !sf_obl_i16(file, &object->pattern) ||
        !sf_obl_i16(file, &object->palette) ||
        !sf_obl_i16(file, &object->opacity) ||
        !sf_obl_i16(file, &object->status) ||
        !sf_obl_i16(file, &object->height) ||
        (version > 0 &&
          (!sf_obl_i16(file, &object->red_strength) ||
           !sf_obl_i16(file, &object->green_strength) ||
           !sf_obl_i16(file, &object->blue_strength))) ||
        !sf_obl_i32(file, &object->judgement.left) ||
        !sf_obl_i32(file, &object->judgement.top) ||
        !sf_obl_i32(file, &object->judgement.right) ||
        !sf_obl_i32(file, &object->judgement.bottom)) goto done;
  }
  map->count = (uint16_t) count;
  map->version = (uint8_t) version;
  success = true;
done:
  fclose(file);
  if (!success) {
    (void) sf_arena_rewind(arena, mark);
    memset(map, 0, sizeof(*map));
  }
  return success;
}
