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

#include "data/gnd.h"

#include "data/rclib.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

typedef struct SfGroundDecode {
  SfGroundCell *cells;
  size_t cell_count;
  uint8_t low_byte;
} SfGroundDecode;

static bool sf_gnd_read(FILE *file, void *output, size_t size) {
  return size == 0u || fread(output, 1u, size, file) == size;
}

static bool sf_gnd_i32(FILE *file, int32_t *value) {
  uint8_t bytes[4];
  uint32_t raw;
  if (!sf_gnd_read(file, bytes, sizeof(bytes))) return false;
  raw = (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8u) |
    ((uint32_t) bytes[2] << 16u) | ((uint32_t) bytes[3] << 24u);
  *value = (int32_t) raw;
  return true;
}

static bool sf_gnd_write_cell_byte(
    void *user, size_t offset, uint8_t value) {
  SfGroundDecode *decode = (SfGroundDecode *) user;
  const size_t plane_bytes = decode->cell_count * 2u;
  size_t plane;
  size_t cell;
  int16_t decoded;
  if (offset >= plane_bytes * 3u) return false;
  if ((offset & 1u) == 0u) {
    decode->low_byte = value;
    return true;
  }
  plane = offset / plane_bytes;
  if (plane == 0u) return true;
  cell = (offset % plane_bytes) / 2u;
  decoded = (int16_t) ((uint16_t) decode->low_byte |
    ((uint16_t) value << 8u));
  if (decoded < -1 || decoded > 254) return false;
  if (plane == 1u)
    decode->cells[cell].pattern_set = decoded < 0
      ? SF_GROUND_EMPTY_PATTERN : (uint8_t) decoded;
  else
    decode->cells[cell].pattern = decoded < 0
      ? SF_GROUND_EMPTY_PATTERN : (uint8_t) decoded;
  return true;
}

static bool sf_gnd_decode_raw(
    FILE *file, size_t decoded_size, SfGroundDecode *decode) {
  size_t offset;
  for (offset = 0u; offset < decoded_size; ++offset) {
    const int value = fgetc(file);
    if (value < 0 || !sf_gnd_write_cell_byte(
          decode, offset, (uint8_t) value)) return false;
  }
  return true;
}

bool sf_gnd_load_render_map(
    const char *path, SfArena *arena, SfGroundMap *map) {
  FILE *file;
  char header[16];
  int32_t width;
  int32_t height;
  int32_t chip_width;
  int32_t chip_height;
  int32_t base_x;
  int32_t base_y;
  size_t cell_count;
  size_t decoded_size;
  size_t mark;
  int compressed;
  SfGroundDecode decode;
  bool success = false;
  if (!path || !arena || !map) return false;
  mark = sf_arena_mark(arena);
  memset(map, 0, sizeof(*map));
  file = fopen(path, "rb");
  if (!file) return false;
  if (!sf_gnd_read(file, header, sizeof(header)) ||
      memcmp(header, "RPGSCRN_GNDv", 12u) != 0 ||
      !sf_gnd_i32(file, &width) || !sf_gnd_i32(file, &height) ||
      !sf_gnd_i32(file, &chip_width) || !sf_gnd_i32(file, &chip_height) ||
      !sf_gnd_i32(file, &base_x) || !sf_gnd_i32(file, &base_y) ||
      width <= 0 || width > UINT16_MAX ||
      height <= 0 || height > UINT16_MAX ||
      chip_width <= 0 || chip_width > UINT16_MAX ||
      chip_height <= 0 || chip_height > UINT16_MAX ||
      (size_t) height > SIZE_MAX / (size_t) width) goto done;
  cell_count = (size_t) width * (size_t) height;
  if (cell_count > SIZE_MAX / 6u) goto done;
  decoded_size = cell_count * 6u;
  map->cells = (SfGroundCell *) sf_arena_push(
    arena, cell_count * sizeof(*map->cells), sizeof(uint16_t));
  if (!map->cells) goto done;
  memset(map->cells, SF_GROUND_EMPTY_PATTERN,
    cell_count * sizeof(*map->cells));
  decode.cells = map->cells;
  decode.cell_count = cell_count;
  decode.low_byte = 0u;
  compressed = fgetc(file);
  if (compressed < 0 ||
      (compressed == 0
        ? !sf_gnd_decode_raw(file, decoded_size, &decode)
        : !sf_rclib_decode_stream_to(
            file, decoded_size, sf_gnd_write_cell_byte, &decode)))
    goto done;
  map->width = (uint16_t) width;
  map->height = (uint16_t) height;
  map->chip_width = (uint16_t) chip_width;
  map->chip_height = (uint16_t) chip_height;
  map->base_magnification_x = base_x;
  map->base_magnification_y = base_y;
  success = true;
done:
  fclose(file);
  if (!success) {
    (void) sf_arena_rewind(arena, mark);
    memset(map, 0, sizeof(*map));
  }
  return success;
}

const SfGroundCell *sf_ground_cell(
    const SfGroundMap *map, int32_t x, int32_t y) {
  if (!map || !map->cells || x < 0 || y < 0 ||
      x >= map->width || y >= map->height) return NULL;
  return &map->cells[(size_t) y * map->width + (size_t) x];
}
