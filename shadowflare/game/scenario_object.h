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

#ifndef SHADOWFLARE_GAME_SCENARIO_OBJECT_H
#define SHADOWFLARE_GAME_SCENARIO_OBJECT_H

#include "core/bounds.h"
#include "core/coordinates.h"
#include "data/mct.h"
#include "game/scenario_entity.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_SCENARIO_OBJECT_CHARACTER_BASE 10000000

typedef struct SfScenarioObject {
  SfWorldPoint position;
  SfObjectBounds judgement;
  int32_t id;
  int32_t resource_id;
  int32_t state[SF_MCT_ENTITY_STATE_COUNT];
  int32_t state_override[SF_MCT_ENTITY_STATE_COUNT];
  int32_t static_pattern;
  int32_t animation_chart;
  int32_t draw_strength;
  int32_t red_strength;
  int32_t green_strength;
  int32_t blue_strength;
  int16_t part_red_strength[SF_MCT_PERSON_PART_LIMIT];
  int16_t part_green_strength[SF_MCT_PERSON_PART_LIMIT];
  int16_t part_blue_strength[SF_MCT_PERSON_PART_LIMIT];
  uint32_t animation_frame;
  int16_t display_status;
  int16_t display_height;
  uint8_t direction;
  uint8_t enabled_parts;
  uint8_t visual_mode;
  uint8_t draw_flags;
  bool state_override_enabled;
} SfScenarioObject;

typedef struct SfScenarioObjectSet {
  SfScenarioObject objects[SF_MCT_OBJECT_LIMIT];
  uint8_t count;
} SfScenarioObjectSet;

void sf_scenario_objects_init(
  SfScenarioObjectSet *objects, const SfMctScenario *scenario);
void sf_scenario_objects_update(SfScenarioObjectSet *objects);
int32_t sf_scenario_object_character_number(const SfScenarioObject *object);
SfScenarioObject *sf_scenario_object_find(
  SfScenarioObjectSet *objects, int32_t character_number);
const SfScenarioObject *sf_scenario_object_find_const(
  const SfScenarioObjectSet *objects, int32_t character_number);
const SfScenarioObject *sf_scenario_object_at(
  const SfScenarioObjectSet *objects, uint8_t index);
bool sf_scenario_object_state(
  const SfScenarioObject *object, SfScenarioEntityChannel channel);
void sf_scenario_object_set_state(
  SfScenarioObject *object, SfScenarioEntityChannel channel, int32_t value);
void sf_scenario_object_set_state_override(
  SfScenarioObject *object, int32_t visible,
  int32_t pointer, int32_t judgement);
bool sf_scenario_object_draw_requested(const SfScenarioObject *object);
int32_t sf_scenario_object_part_red(
  const SfScenarioObject *object, uint8_t part, bool hovered);
int32_t sf_scenario_object_part_green(
  const SfScenarioObject *object, uint8_t part, bool hovered);
int32_t sf_scenario_object_part_blue(
  const SfScenarioObject *object, uint8_t part, bool hovered);

#endif
