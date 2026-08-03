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

typedef struct SfGroundMovementDecode {
  uint8_t *flags;
  size_t count;
  uint8_t low_byte;
} SfGroundMovementDecode;

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

static bool sf_gnd_write_movement_byte(
    void *user, size_t offset, uint8_t value) {
  SfGroundMovementDecode *decode = (SfGroundMovementDecode *) user;
  size_t cell;
  int16_t decoded;
  if (offset >= decode->count * 2u) return false;
  if ((offset & 1u) == 0u) {
    decode->low_byte = value;
    return true;
  }
  cell = offset / 2u;
  decoded = (int16_t) ((uint16_t) decode->low_byte |
    ((uint16_t) value << 8u));
  decode->flags[cell / 4u] |=
    (uint8_t) (((uint16_t) decoded & 3u) << ((cell & 3u) * 2u));
  return true;
}

static bool sf_gnd_decode_raw(
    FILE *file, size_t decoded_size,
    SfRclibByteSink writer, void *user) {
  size_t offset;
  for (offset = 0u; offset < decoded_size; ++offset) {
    const int value = fgetc(file);
    if (value < 0 || !writer(user, offset, (uint8_t) value)) return false;
  }
  return true;
}

static bool sf_gnd_decode_block(
    FILE *file, size_t decoded_size,
    SfRclibByteSink writer, void *user) {
  const int compressed = fgetc(file);
  return compressed >= 0 && (compressed == 0
    ? sf_gnd_decode_raw(file, decoded_size, writer, user)
    : sf_rclib_decode_stream_to(file, decoded_size, writer, user));
}

static int64_t sf_gnd_floor_divide(int64_t numerator, int64_t denominator) {
  int64_t result = numerator / denominator;
  if (numerator < 0 && numerator % denominator != 0) --result;
  return result;
}

bool sf_gnd_load(
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
  size_t judge_count;
  size_t movement_bytes;
  size_t mark;
  SfGroundDecode decode;
  SfGroundMovementDecode movement_decode;
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
  if (!sf_gnd_decode_block(
        file, decoded_size, sf_gnd_write_cell_byte, &decode)) goto done;
  map->width = (uint16_t) width;
  map->height = (uint16_t) height;
  map->chip_width = (uint16_t) chip_width;
  map->chip_height = (uint16_t) chip_height;
  map->base_magnification_x = base_x;
  map->base_magnification_y = base_y;
  {
    const int64_t real_base_x = (int64_t) base_x * 15 / 100;
    const int64_t real_base_y = (int64_t) base_y * 10 / 100;
    const int64_t divisor = real_base_x * real_base_y * 2;
    const int64_t map_width = (int64_t) width * chip_width;
    const int64_t map_height = (int64_t) height * chip_height;
    int64_t right_y;
    int64_t left_x;
    int64_t left_y;
    int64_t bottom_x;
    int64_t judge_width;
    int64_t judge_height;
    if (real_base_x <= 0 || real_base_y <= 0 || divisor <= 0) goto done;
    right_y = sf_gnd_floor_divide(-real_base_y * map_width, divisor);
    left_x = sf_gnd_floor_divide(real_base_x * map_height, divisor);
    left_y = left_x;
    bottom_x = sf_gnd_floor_divide(
      real_base_x * map_height + real_base_y * map_width, divisor);
    judge_width = bottom_x + 2;
    judge_height = left_y - right_y + 2;
    if (judge_width <= 0 || judge_height <= 0 ||
        judge_width > INT32_MAX || judge_height > INT32_MAX ||
        (uint64_t) judge_width > SIZE_MAX / (uint64_t) judge_height)
      goto done;
    map->judge_width = (int32_t) judge_width;
    map->judge_height = (int32_t) judge_height;
    map->judge_offset_x = -1;
    map->judge_offset_y = (int32_t) right_y - 1;
  }
  judge_count = (size_t) map->judge_width * (size_t) map->judge_height;
  if (judge_count > SIZE_MAX - 3u || judge_count > SIZE_MAX / 2u) goto done;
  movement_bytes = (judge_count + 3u) / 4u;
  map->movement_flags = (uint8_t *) sf_arena_push_zero(
    arena, movement_bytes, sizeof(uint8_t));
  if (!map->movement_flags) goto done;
  movement_decode.flags = map->movement_flags;
  movement_decode.count = judge_count;
  movement_decode.low_byte = 0u;
  if (!sf_gnd_decode_block(
        file, judge_count * 2u,
        sf_gnd_write_movement_byte, &movement_decode)) goto done;
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

uint8_t sf_ground_movement_flags(
    const SfGroundMap *map, int32_t x, int32_t y) {
  size_t index;
  const int32_t local_x = map ? x - map->judge_offset_x : -1;
  const int32_t local_y = map ? y - map->judge_offset_y : -1;
  if (!map || !map->movement_flags || local_x < 0 || local_y < 0 ||
      local_x >= map->judge_width || local_y >= map->judge_height) return 0u;
  index = (size_t) local_y * (size_t) map->judge_width + (size_t) local_x;
  return (uint8_t) ((map->movement_flags[index / 4u] >>
    ((index & 3u) * 2u)) & 3u);
}
