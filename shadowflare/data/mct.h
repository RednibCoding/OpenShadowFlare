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

#include "core/arena.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_MCT_ENTRY_LIMIT 64u
#define SF_MCT_OBJECT_LIMIT 128u
#define SF_MCT_PERSON_LIMIT 32u
#define SF_MCT_PERSON_NAME_CAPACITY 64u
#define SF_MCT_PERSON_PART_LIMIT 8u
#define SF_MCT_ENTITY_STATE_COUNT 3u

typedef struct SfMctEntry {
  int32_t key;
  int32_t world_x;
  int32_t world_y;
  int32_t direction;
} SfMctEntry;

typedef struct SfMctObject {
  char name[SF_MCT_PERSON_NAME_CAPACITY];
  int32_t id;
  int32_t resource_id;
  uint32_t name_color;
  int32_t label_height;
  int32_t world_x;
  int32_t world_y;
  int32_t judgement_left;
  int32_t judgement_top;
  int32_t judgement_right;
  int32_t judgement_bottom;
  int32_t direction;
  int32_t initial_state[SF_MCT_ENTITY_STATE_COUNT];
  int32_t visual_mode;
  int32_t static_pattern;
  int32_t animation_chart;
  int32_t draw_status_bit_80;
  int32_t height;
  int32_t draw_flags;
  int32_t draw_strength;
  int32_t red_strength;
  int32_t green_strength;
  int32_t blue_strength;
  int16_t part_red_strength[SF_MCT_PERSON_PART_LIMIT];
  int16_t part_green_strength[SF_MCT_PERSON_PART_LIMIT];
  int16_t part_blue_strength[SF_MCT_PERSON_PART_LIMIT];
  uint8_t part_visibility[SF_MCT_PERSON_PART_LIMIT];
  uint8_t custom_part_count;
  bool custom_parts;
} SfMctObject;

typedef struct SfMctPerson {
  char name[SF_MCT_PERSON_NAME_CAPACITY];
  int32_t id;
  int32_t resource_id;
  uint32_t name_color;
  int32_t label_height;
  int32_t world_x;
  int32_t world_y;
  int32_t judgement_left;
  int32_t judgement_top;
  int32_t judgement_right;
  int32_t judgement_bottom;
  int32_t direction;
  int32_t initial_state[SF_MCT_ENTITY_STATE_COUNT];
  int32_t walk_speed;
  int32_t walk_duration;
  int32_t idle_duration;
  int32_t wander_left;
  int32_t wander_top;
  int32_t wander_right;
  int32_t wander_bottom;
  int32_t reserved_behavior_value;
  int16_t red_strength[SF_MCT_PERSON_PART_LIMIT];
  int16_t green_strength[SF_MCT_PERSON_PART_LIMIT];
  int16_t blue_strength[SF_MCT_PERSON_PART_LIMIT];
  uint8_t part_visibility[SF_MCT_PERSON_PART_LIMIT];
  uint8_t custom_part_count;
  bool custom_parts;
  bool wander_bounds_relative;
  bool wandering_enabled;
  bool scripted_turning_enabled;
} SfMctPerson;

typedef struct SfMctScenario {
  char map_path[260];
  char title[256];
  SfMctEntry *entries;
  SfMctObject *objects;
  SfMctPerson *people;
  int32_t music_track;
  uint8_t entry_count;
  uint8_t object_count;
  uint8_t people_count;
} SfMctScenario;

bool sf_mct_load(
  const char *path, SfArena *arena, SfMctScenario *scenario);
const SfMctEntry *sf_mct_find_entry(
  const SfMctScenario *scenario, int32_t key);

#endif
