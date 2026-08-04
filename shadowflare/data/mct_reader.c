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

#include "data/mct_reader.h"

#include <string.h>

#define SF_MCT_STRING_LIMIT 65535u

bool sf_mct_reader_read(FILE *file, void *output, size_t size) {
  return size == 0u || fread(output, 1u, size, file) == size;
}

bool sf_mct_reader_skip(FILE *file, uint64_t size) {
  while (size > 0u) {
    const long amount = size > 0x7fffffffULL ? 0x7fffffffL : (long) size;
    if (fseek(file, amount, SEEK_CUR) != 0) return false;
    size -= (uint64_t) amount;
  }
  return true;
}

bool sf_mct_reader_u32(FILE *file, uint32_t *value) {
  uint8_t bytes[4];
  if (!sf_mct_reader_read(file, bytes, sizeof(bytes))) return false;
  *value = (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8u) |
    ((uint32_t) bytes[2] << 16u) | ((uint32_t) bytes[3] << 24u);
  return true;
}

bool sf_mct_reader_i32(FILE *file, int32_t *value) {
  uint32_t raw;
  if (!sf_mct_reader_u32(file, &raw)) return false;
  *value = (int32_t) raw;
  return true;
}

bool sf_mct_reader_i16(FILE *file, int16_t *value) {
  uint8_t bytes[2];
  if (!sf_mct_reader_read(file, bytes, sizeof(bytes))) return false;
  *value = (int16_t) ((uint16_t) bytes[0] | ((uint16_t) bytes[1] << 8u));
  return true;
}

bool sf_mct_reader_skip_values(FILE *file, uint32_t size) {
  uint32_t count;
  return sf_mct_reader_u32(file, &count) && count <= SF_MCT_ENTITY_LIMIT &&
    sf_mct_reader_skip(file, (uint64_t) count * size);
}

bool sf_mct_reader_string(
    FILE *file, uint32_t size, char *output, size_t capacity) {
  const size_t kept = size < capacity ? size : capacity - 1u;
  if (!output || capacity == 0u ||
      !sf_mct_reader_read(file, output, kept) ||
      !sf_mct_reader_skip(file, size - kept)) return false;
  output[kept] = '\0';
  return size < capacity;
}

static bool sf_mct_reader_parts(
    FILE *file, SfMctCommonEntity *entity, uint32_t count) {
  uint32_t index;
  if (count > SF_MCT_ENTITY_LIMIT) return false;
  entity->custom_parts = true;
  entity->custom_part_count = count > SF_MCT_PERSON_PART_LIMIT
    ? SF_MCT_PERSON_PART_LIMIT : (uint8_t) count;
  for (index = 0u; index < count; ++index) {
    int32_t value;
    if (!sf_mct_reader_i32(file, &value)) return false;
    if (index < SF_MCT_PERSON_PART_LIMIT)
      entity->part_visibility[index] = value != 0 ? 1u : 0u;
  }
  for (index = 0u; index < count; ++index) {
    int16_t value;
    if (!sf_mct_reader_i16(file, &value)) return false;
    if (index < SF_MCT_PERSON_PART_LIMIT) entity->red_strength[index] = value;
  }
  for (index = 0u; index < count; ++index) {
    int16_t value;
    if (!sf_mct_reader_i16(file, &value)) return false;
    if (index < SF_MCT_PERSON_PART_LIMIT) entity->green_strength[index] = value;
  }
  for (index = 0u; index < count; ++index) {
    int16_t value;
    if (!sf_mct_reader_i16(file, &value)) return false;
    if (index < SF_MCT_PERSON_PART_LIMIT) entity->blue_strength[index] = value;
  }
  return true;
}

bool sf_mct_reader_common(FILE *file, SfMctCommonEntity *entity) {
  uint32_t name_size;
  uint32_t state_count;
  uint32_t part_count;
  int32_t custom_parts;
  int32_t ignored;
  uint32_t index;
  memset(entity, 0, sizeof(*entity));
  for (index = 0u; index < SF_MCT_PERSON_PART_LIMIT; ++index) {
    entity->part_visibility[index] = 1u;
    entity->red_strength[index] = 1000;
    entity->green_strength[index] = 1000;
    entity->blue_strength[index] = 1000;
  }
  if (!sf_mct_reader_i32(file, &entity->id) ||
      !sf_mct_reader_i32(file, &entity->resource_id) ||
      !sf_mct_reader_u32(file, &name_size) ||
      name_size > SF_MCT_STRING_LIMIT ||
      !sf_mct_reader_string(
        file, name_size, entity->name, sizeof(entity->name)) ||
      (name_size > 0u && !sf_mct_reader_u32(file, &entity->name_color)) ||
      !sf_mct_reader_i32(file, &entity->label_height) ||
      !sf_mct_reader_i32(file, &entity->world_x) ||
      !sf_mct_reader_i32(file, &entity->world_y) ||
      !sf_mct_reader_i32(file, &entity->judgement_left) ||
      !sf_mct_reader_i32(file, &entity->judgement_top) ||
      !sf_mct_reader_i32(file, &entity->judgement_right) ||
      !sf_mct_reader_i32(file, &entity->judgement_bottom) ||
      !sf_mct_reader_i32(file, &entity->direction) ||
      !sf_mct_reader_u32(file, &state_count) ||
      state_count != SF_MCT_ENTITY_STATE_COUNT) return false;
  for (index = 0u; index < state_count; ++index)
    if (!sf_mct_reader_i32(file, &entity->initial_state[index])) return false;
  if (!sf_mct_reader_i32(file, &custom_parts) ||
      (custom_parts != 0 && custom_parts != 1)) return false;
  if (custom_parts == 1 &&
      (!sf_mct_reader_u32(file, &part_count) ||
       !sf_mct_reader_parts(file, entity, part_count))) return false;
  return sf_mct_reader_i32(file, &ignored);
}
