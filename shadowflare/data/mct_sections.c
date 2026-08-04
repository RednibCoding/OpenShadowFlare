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

#include "data/mct_sections.h"

#include "data/mct_reader.h"

#include <string.h>

static void sf_mct_copy_object_common(
    SfMctObject *object, const SfMctCommonEntity *common) {
  memcpy(object->name, common->name, sizeof(object->name));
  object->id = common->id;
  object->resource_id = common->resource_id;
  object->name_color = common->name_color;
  object->label_height = common->label_height;
  object->world_x = common->world_x;
  object->world_y = common->world_y;
  object->judgement_left = common->judgement_left;
  object->judgement_top = common->judgement_top;
  object->judgement_right = common->judgement_right;
  object->judgement_bottom = common->judgement_bottom;
  object->direction = common->direction;
  memcpy(object->initial_state, common->initial_state,
    sizeof(object->initial_state));
  memcpy(object->part_red_strength, common->red_strength,
    sizeof(object->part_red_strength));
  memcpy(object->part_green_strength, common->green_strength,
    sizeof(object->part_green_strength));
  memcpy(object->part_blue_strength, common->blue_strength,
    sizeof(object->part_blue_strength));
  memcpy(object->part_visibility, common->part_visibility,
    sizeof(object->part_visibility));
  object->custom_part_count = common->custom_part_count;
  object->custom_parts = common->custom_parts;
}

static bool sf_mct_read_objects(
    FILE *file, SfArena *arena, SfMctScenario *scenario) {
  uint32_t count;
  uint32_t index;
  if (!sf_mct_reader_u32(file, &count) || count > SF_MCT_OBJECT_LIMIT)
    return false;
  scenario->objects = (SfMctObject *) sf_arena_push_zero(
    arena, (size_t) count * sizeof(*scenario->objects), sizeof(void *));
  if (count > 0u && !scenario->objects) return false;
  for (index = 0u; index < count; ++index) {
    SfMctCommonEntity common;
    SfMctObject *object = &scenario->objects[index];
    int32_t fields[13];
    uint32_t field;
    memset(object, 0, sizeof(*object));
    if (!sf_mct_reader_common(file, &common)) return false;
    for (field = 0u; field < 13u; ++field)
      if (!sf_mct_reader_i32(file, &fields[field])) return false;
    sf_mct_copy_object_common(object, &common);
    object->visual_mode = fields[0];
    object->static_pattern = fields[1];
    object->animation_chart = fields[2];
    object->draw_status_bit_80 = fields[3];
    object->height = fields[4];
    object->draw_flags = fields[7];
    object->draw_strength = fields[8];
    object->red_strength = fields[10];
    object->green_strength = fields[11];
    object->blue_strength = fields[12];
  }
  scenario->object_count = (uint8_t) count;
  return true;
}

static void sf_mct_copy_person_common(
    SfMctPerson *person, const SfMctCommonEntity *common) {
  memcpy(person->name, common->name, sizeof(person->name));
  person->id = common->id;
  person->resource_id = common->resource_id;
  person->name_color = common->name_color;
  person->label_height = common->label_height;
  person->world_x = common->world_x;
  person->world_y = common->world_y;
  person->judgement_left = common->judgement_left;
  person->judgement_top = common->judgement_top;
  person->judgement_right = common->judgement_right;
  person->judgement_bottom = common->judgement_bottom;
  person->direction = common->direction;
  memcpy(person->initial_state, common->initial_state,
    sizeof(person->initial_state));
  memcpy(person->red_strength, common->red_strength,
    sizeof(person->red_strength));
  memcpy(person->green_strength, common->green_strength,
    sizeof(person->green_strength));
  memcpy(person->blue_strength, common->blue_strength,
    sizeof(person->blue_strength));
  memcpy(person->part_visibility, common->part_visibility,
    sizeof(person->part_visibility));
  person->custom_part_count = common->custom_part_count;
  person->custom_parts = common->custom_parts;
}

static bool sf_mct_read_people(
    FILE *file, SfArena *arena, SfMctScenario *scenario) {
  uint32_t count;
  uint32_t index;
  if (!sf_mct_reader_u32(file, &count) || count > SF_MCT_PERSON_LIMIT)
    return false;
  scenario->people = (SfMctPerson *) sf_arena_push_zero(
    arena, (size_t) count * sizeof(*scenario->people), sizeof(void *));
  if (count > 0u && !scenario->people) return false;
  for (index = 0u; index < count; ++index) {
    SfMctCommonEntity common;
    SfMctPerson *person = &scenario->people[index];
    int32_t fields[11];
    uint32_t field;
    memset(person, 0, sizeof(*person));
    if (!sf_mct_reader_common(file, &common) ||
        common.direction < 0 || common.direction > 7) return false;
    for (field = 0u; field < 11u; ++field)
      if (!sf_mct_reader_i32(file, &fields[field])) return false;
    sf_mct_copy_person_common(person, &common);
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
  }
  scenario->people_count = (uint8_t) count;
  return true;
}

static bool sf_mct_skip_entity_group(FILE *file, uint32_t tail_size) {
  uint32_t count;
  uint32_t index;
  if (!sf_mct_reader_u32(file, &count) || count > SF_MCT_ENTITY_LIMIT)
    return false;
  for (index = 0u; index < count; ++index) {
    SfMctCommonEntity common;
    if (!sf_mct_reader_common(file, &common) ||
        !sf_mct_reader_skip(file, tail_size)) return false;
  }
  return true;
}

static bool sf_mct_read_entries(
    FILE *file, SfArena *arena, SfMctScenario *scenario) {
  uint32_t count;
  uint32_t index;
  if (!sf_mct_reader_u32(file, &count) || count > SF_MCT_ENTRY_LIMIT)
    return false;
  scenario->entries = (SfMctEntry *) sf_arena_push_zero(
    arena, (size_t) count * sizeof(*scenario->entries), sizeof(void *));
  if (count > 0u && !scenario->entries) return false;
  for (index = 0u; index < count; ++index) {
    SfMctEntry *entry = &scenario->entries[index];
    if (!sf_mct_reader_i32(file, &entry->key) ||
        !sf_mct_reader_i32(file, &entry->world_x) ||
        !sf_mct_reader_i32(file, &entry->world_y) ||
        !sf_mct_reader_i32(file, &entry->direction) ||
        entry->direction < 0 || entry->direction > 7) return false;
  }
  scenario->entry_count = (uint8_t) count;
  return true;
}

bool sf_mct_read_sections(
    FILE *file, SfArena *arena, SfMctScenario *scenario) {
  return sf_mct_reader_skip_values(file, 4u) &&
    sf_mct_reader_skip_values(file, 4u) &&
    sf_mct_reader_skip_values(file, 4u) &&
    sf_mct_read_objects(file, arena, scenario) &&
    sf_mct_read_people(file, arena, scenario) &&
    sf_mct_skip_entity_group(file, 0x13cu) &&
    sf_mct_skip_entity_group(file, 0x10u) &&
    sf_mct_read_entries(file, arena, scenario);
}
