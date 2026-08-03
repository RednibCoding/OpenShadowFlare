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

#include <stdio.h>
#include <string.h>

#define SF_MCT_FIXED_HEADER_SIZE 0x324L
#define SF_MCT_MAP_PATH_OFFSET 0x114L
#define SF_MCT_MUSIC_OFFSET 0x220L
#define SF_MCT_TITLE_OFFSET 0x224L
#define SF_MCT_ENTITY_LIMIT 4096u
#define SF_MCT_STRING_LIMIT 65535u

static bool sf_mct_read(FILE *file, void *output, size_t size) {
  return size == 0u || fread(output, 1u, size, file) == size;
}

static bool sf_mct_skip(FILE *file, uint64_t size) {
  while (size > 0u) {
    const long amount = size > 0x7fffffffULL ? 0x7fffffffL : (long) size;
    if (fseek(file, amount, SEEK_CUR) != 0) return false;
    size -= (uint64_t) amount;
  }
  return true;
}

static bool sf_mct_u32(FILE *file, uint32_t *value) {
  uint8_t bytes[4];
  if (!sf_mct_read(file, bytes, sizeof(bytes))) return false;
  *value = (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8u) |
    ((uint32_t) bytes[2] << 16u) | ((uint32_t) bytes[3] << 24u);
  return true;
}

static bool sf_mct_i32(FILE *file, int32_t *value) {
  uint32_t raw;
  if (!sf_mct_u32(file, &raw)) return false;
  *value = (int32_t) raw;
  return true;
}

static bool sf_mct_i16(FILE *file, int16_t *value) {
  uint8_t bytes[2];
  if (!sf_mct_read(file, bytes, sizeof(bytes))) return false;
  *value = (int16_t) (
    (uint16_t) bytes[0] | ((uint16_t) bytes[1] << 8u));
  return true;
}

static bool sf_mct_skip_values(FILE *file, uint32_t size, uint32_t limit) {
  uint32_t count;
  return sf_mct_u32(file, &count) && count <= limit &&
    sf_mct_skip(file, (uint64_t) count * size);
}

static bool sf_mct_skip_common_entity(FILE *file) {
  uint32_t name_size;
  uint32_t state_count;
  uint32_t part_count;
  int32_t custom_parts;
  if (!sf_mct_skip(file, 8u) || !sf_mct_u32(file, &name_size) ||
      name_size > SF_MCT_STRING_LIMIT || !sf_mct_skip(file, name_size) ||
      (name_size > 0u && !sf_mct_skip(file, 4u)) ||
      !sf_mct_skip(file, 32u) || !sf_mct_u32(file, &state_count) ||
      state_count > SF_MCT_ENTITY_LIMIT ||
      !sf_mct_skip(file, (uint64_t) state_count * 4u) ||
      !sf_mct_i32(file, &custom_parts) ||
      (custom_parts != 0 && custom_parts != 1)) return false;
  if (custom_parts == 1) {
    if (!sf_mct_u32(file, &part_count) ||
        part_count > SF_MCT_ENTITY_LIMIT ||
        !sf_mct_skip(file, (uint64_t) part_count * 10u)) return false;
  }
  return sf_mct_skip(file, 4u);
}

static bool sf_mct_skip_entity_group(FILE *file, uint32_t tail_size) {
  uint32_t count;
  uint32_t index;
  if (!sf_mct_u32(file, &count) || count > SF_MCT_ENTITY_LIMIT) return false;
  for (index = 0u; index < count; ++index) {
    if (!sf_mct_skip_common_entity(file) ||
        !sf_mct_skip(file, tail_size)) return false;
  }
  return true;
}

static bool sf_mct_read_string(
    FILE *file, uint32_t size, char *output, size_t capacity) {
  const size_t kept = size < capacity ? size : capacity - 1u;
  if (!output || capacity == 0u ||
      !sf_mct_read(file, output, kept) ||
      !sf_mct_skip(file, size - kept)) return false;
  output[kept] = '\0';
  return size < capacity;
}

static bool sf_mct_read_person_parts(
    FILE *file, SfMctPerson *person, uint32_t part_count) {
  uint32_t part;
  if (part_count > SF_MCT_ENTITY_LIMIT) return false;
  person->custom_parts = true;
  person->custom_part_count = part_count > SF_MCT_PERSON_PART_LIMIT
    ? SF_MCT_PERSON_PART_LIMIT : (uint8_t) part_count;
  for (part = 0u; part < part_count; ++part) {
    int32_t visible;
    if (!sf_mct_i32(file, &visible)) return false;
    if (part < SF_MCT_PERSON_PART_LIMIT)
      person->part_visibility[part] = visible != 0 ? 1u : 0u;
  }
  for (part = 0u; part < part_count; ++part) {
    int16_t strength;
    if (!sf_mct_i16(file, &strength)) return false;
    if (part < SF_MCT_PERSON_PART_LIMIT)
      person->red_strength[part] = strength;
  }
  for (part = 0u; part < part_count; ++part) {
    int16_t strength;
    if (!sf_mct_i16(file, &strength)) return false;
    if (part < SF_MCT_PERSON_PART_LIMIT)
      person->green_strength[part] = strength;
  }
  for (part = 0u; part < part_count; ++part) {
    int16_t strength;
    if (!sf_mct_i16(file, &strength)) return false;
    if (part < SF_MCT_PERSON_PART_LIMIT)
      person->blue_strength[part] = strength;
  }
  return true;
}

static bool sf_mct_read_person(FILE *file, SfMctPerson *person) {
  uint32_t name_size;
  uint32_t state_count;
  uint32_t part_count;
  int32_t custom_parts;
  int32_t ignored_common;
  int32_t fields[11];
  uint32_t index;
  memset(person, 0, sizeof(*person));
  for (index = 0u; index < SF_MCT_PERSON_PART_LIMIT; ++index) {
    person->part_visibility[index] = 1u;
    person->red_strength[index] = 1000;
    person->green_strength[index] = 1000;
    person->blue_strength[index] = 1000;
  }
  if (!sf_mct_i32(file, &person->id) ||
      !sf_mct_i32(file, &person->resource_id) ||
      !sf_mct_u32(file, &name_size) ||
      name_size > SF_MCT_STRING_LIMIT ||
      !sf_mct_read_string(
        file, name_size, person->name, sizeof(person->name)) ||
      (name_size > 0u && !sf_mct_u32(file, &person->name_color)) ||
      !sf_mct_i32(file, &person->label_height) ||
      !sf_mct_i32(file, &person->world_x) ||
      !sf_mct_i32(file, &person->world_y) ||
      !sf_mct_i32(file, &person->judgement_left) ||
      !sf_mct_i32(file, &person->judgement_top) ||
      !sf_mct_i32(file, &person->judgement_right) ||
      !sf_mct_i32(file, &person->judgement_bottom) ||
      !sf_mct_i32(file, &person->direction) ||
      person->direction < 0 || person->direction > 7 ||
      !sf_mct_u32(file, &state_count) ||
      state_count != SF_MCT_ENTITY_STATE_COUNT) return false;
  for (index = 0u; index < state_count; ++index) {
    if (!sf_mct_i32(file, &person->initial_state[index])) return false;
  }
  if (!sf_mct_i32(file, &custom_parts) ||
      (custom_parts != 0 && custom_parts != 1)) return false;
  if (custom_parts == 1 &&
      (!sf_mct_u32(file, &part_count) ||
       !sf_mct_read_person_parts(file, person, part_count))) return false;
  if (!sf_mct_i32(file, &ignored_common)) return false;
  for (index = 0u; index < 11u; ++index) {
    if (!sf_mct_i32(file, &fields[index])) return false;
  }
  person->walk_speed = fields[0];
  person->walk_duration = fields[1];
  person->idle_duration = fields[2];
  person->wander_bounds_relative = fields[3] == 0;
  person->wander_left = fields[4];
  person->wander_top = fields[5];
  person->wander_right = fields[6];
  person->wander_bottom = fields[7];
  person->scripted_turning_enabled = fields[8] != 0;
  person->wandering_enabled = fields[9] == 0;
  person->reserved_behavior_value = fields[10];
  return true;
}

static bool sf_mct_read_people(FILE *file, SfMctScenario *scenario) {
  uint32_t count;
  uint32_t index;
  if (!sf_mct_u32(file, &count) || count > SF_MCT_PERSON_LIMIT) return false;
  for (index = 0u; index < count; ++index) {
    if (!sf_mct_read_person(file, &scenario->people[index])) return false;
  }
  scenario->people_count = (uint8_t) count;
  return true;
}

static bool sf_mct_read_fixed_string(
    FILE *file, long offset, char *output, size_t capacity) {
  size_t index;
  if (fseek(file, offset, SEEK_SET) != 0 ||
      !sf_mct_read(file, output, capacity)) return false;
  output[capacity - 1u] = '\0';
  for (index = 0u; index < capacity; ++index) {
    if (output[index] == '\0') return true;
  }
  return false;
}

bool sf_mct_load(const char *path, SfMctScenario *scenario) {
  static const uint8_t expected_header[16] = {
    'M', 'C', 'E', 'D', ' ', 'D', 'A', 'T',
    'A', ' ', 'v', '0', '0', '0', '0', 0x1a
  };
  FILE *file;
  uint8_t header[16];
  uint32_t entry_count;
  uint32_t index;
  bool success = false;
  if (!path || !scenario) return false;
  memset(scenario, 0, sizeof(*scenario));
  scenario->music_track = -1;
  file = fopen(path, "rb");
  if (!file) return false;
  if (!sf_mct_read(file, header, sizeof(header)) ||
      memcmp(header, expected_header, sizeof(header)) != 0 ||
      !sf_mct_read_fixed_string(
        file, SF_MCT_MAP_PATH_OFFSET,
        scenario->map_path, sizeof(scenario->map_path)) ||
      fseek(file, SF_MCT_MUSIC_OFFSET, SEEK_SET) != 0 ||
      !sf_mct_i32(file, &scenario->music_track) ||
      !sf_mct_read_fixed_string(
        file, SF_MCT_TITLE_OFFSET,
        scenario->title, sizeof(scenario->title)) ||
      fseek(file, SF_MCT_FIXED_HEADER_SIZE, SEEK_SET) != 0 ||
      !sf_mct_skip_values(file, 4u, SF_MCT_ENTITY_LIMIT) ||
      !sf_mct_skip_values(file, 4u, SF_MCT_ENTITY_LIMIT) ||
      !sf_mct_skip_values(file, 4u, SF_MCT_ENTITY_LIMIT) ||
      !sf_mct_skip_entity_group(file, 0x34u) ||
      !sf_mct_read_people(file, scenario) ||
      !sf_mct_skip_entity_group(file, 0x13cu) ||
      !sf_mct_skip_entity_group(file, 0x10u) ||
      !sf_mct_u32(file, &entry_count) ||
      entry_count > SF_MCT_ENTRY_LIMIT) goto done;
  for (index = 0u; index < entry_count; ++index) {
    SfMctEntry *entry = &scenario->entries[index];
    if (!sf_mct_i32(file, &entry->key) ||
        !sf_mct_i32(file, &entry->world_x) ||
        !sf_mct_i32(file, &entry->world_y) ||
        !sf_mct_i32(file, &entry->direction) ||
        entry->direction < 0 || entry->direction > 7) goto done;
  }
  scenario->entry_count = (uint8_t) entry_count;
  success = true;
done:
  fclose(file);
  if (!success) memset(scenario, 0, sizeof(*scenario));
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
