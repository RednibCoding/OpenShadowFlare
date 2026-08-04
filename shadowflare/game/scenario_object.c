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

#include "game/scenario_object.h"

#include <limits.h>
#include <string.h>

static int16_t sf_scenario_object_i16(int32_t value) {
  return value < INT16_MIN ? INT16_MIN :
    value > INT16_MAX ? INT16_MAX : (int16_t) value;
}

void sf_scenario_objects_init(
    SfScenarioObjectSet *objects, const SfMctScenario *scenario) {
  uint8_t index;
  if (!objects) return;
  memset(objects, 0, sizeof(*objects));
  if (!scenario) return;
  objects->count = scenario->object_count;
  for (index = 0u; index < objects->count; ++index) {
    const SfMctObject *source = &scenario->objects[index];
    SfScenarioObject *object = &objects->objects[index];
    uint8_t part;
    object->position = (SfWorldPoint) {source->world_x, source->world_y};
    object->judgement = (SfObjectBounds) {
      source->judgement_left, source->judgement_top,
      source->judgement_right, source->judgement_bottom};
    object->id = source->id;
    object->resource_id = source->resource_id;
    memcpy(object->state, source->initial_state, sizeof(object->state));
    object->static_pattern = source->static_pattern;
    object->animation_chart = source->animation_chart;
    object->draw_strength = source->draw_strength;
    object->red_strength = source->red_strength;
    object->green_strength = source->green_strength;
    object->blue_strength = source->blue_strength;
    memcpy(object->part_red_strength, source->part_red_strength,
      sizeof(object->part_red_strength));
    memcpy(object->part_green_strength, source->part_green_strength,
      sizeof(object->part_green_strength));
    memcpy(object->part_blue_strength, source->part_blue_strength,
      sizeof(object->part_blue_strength));
    object->display_status = sf_scenario_object_i16(
      source->draw_flags | (source->draw_status_bit_80 != 0 ? 0x80 : 0));
    object->display_height = sf_scenario_object_i16(source->height * 20 / 100);
    object->direction = source->direction < 0 ? 0u :
      source->direction > 8 ? 8u : (uint8_t) source->direction;
    object->visual_mode = source->visual_mode == 0 ? 0u : 1u;
    object->draw_flags = (uint8_t) source->draw_flags;
    for (part = 0u; part < SF_MCT_PERSON_PART_LIMIT; ++part) {
      if (!source->custom_parts || source->part_visibility[part] != 0u)
        object->enabled_parts = (uint8_t) (
          object->enabled_parts | (uint8_t) (1u << part));
    }
  }
}

void sf_scenario_objects_update(SfScenarioObjectSet *objects) {
  uint8_t index;
  if (!objects) return;
  for (index = 0u; index < objects->count; ++index) {
    SfScenarioObject *object = &objects->objects[index];
    if (sf_scenario_object_draw_requested(object) &&
        object->visual_mode == 0u && object->animation_chart >= 0)
      ++object->animation_frame;
  }
}

int32_t sf_scenario_object_character_number(const SfScenarioObject *object) {
  return object ? SF_SCENARIO_OBJECT_CHARACTER_BASE + object->id : INT32_MIN;
}

SfScenarioObject *sf_scenario_object_find(
    SfScenarioObjectSet *objects, int32_t character_number) {
  uint8_t index;
  if (!objects) return NULL;
  for (index = 0u; index < objects->count; ++index)
    if (sf_scenario_object_character_number(&objects->objects[index]) ==
        character_number) return &objects->objects[index];
  return NULL;
}

const SfScenarioObject *sf_scenario_object_find_const(
    const SfScenarioObjectSet *objects, int32_t character_number) {
  uint8_t index;
  if (!objects) return NULL;
  for (index = 0u; index < objects->count; ++index)
    if (sf_scenario_object_character_number(&objects->objects[index]) ==
        character_number) return &objects->objects[index];
  return NULL;
}

const SfScenarioObject *sf_scenario_object_at(
    const SfScenarioObjectSet *objects, uint8_t index) {
  return objects && index < objects->count ? &objects->objects[index] : NULL;
}

bool sf_scenario_object_state(
    const SfScenarioObject *object, SfScenarioEntityChannel channel) {
  return object && channel < SF_MCT_ENTITY_STATE_COUNT &&
    (object->state_override_enabled
      ? object->state_override[channel] : object->state[channel]) != 0;
}

void sf_scenario_object_set_state_override(
    SfScenarioObject *object, int32_t visible,
    int32_t pointer, int32_t judgement) {
  if (!object) return;
  object->state_override[SF_SCENARIO_VISIBLE] = visible;
  object->state_override[SF_SCENARIO_POINTER] = pointer;
  object->state_override[SF_SCENARIO_JUDGEMENT] = judgement;
  object->state_override_enabled = true;
}

void sf_scenario_object_set_state(
    SfScenarioObject *object, SfScenarioEntityChannel channel, int32_t value) {
  if (object && channel < SF_MCT_ENTITY_STATE_COUNT)
    object->state[channel] = value;
}

bool sf_scenario_object_draw_requested(const SfScenarioObject *object) {
  return sf_scenario_object_state(object, SF_SCENARIO_VISIBLE) &&
    ((object->draw_flags & 4u) != 0u || object->draw_strength != 0);
}

static int32_t sf_scenario_object_part_strength(
    int16_t part_strength, int32_t global_strength, bool hovered) {
  return (int32_t) ((int64_t) part_strength * global_strength / 1000) +
    (hovered ? 300 : 0);
}

int32_t sf_scenario_object_part_red(
    const SfScenarioObject *object, uint8_t part, bool hovered) {
  return object && part < SF_MCT_PERSON_PART_LIMIT
    ? sf_scenario_object_part_strength(
        object->part_red_strength[part], object->red_strength, hovered) : 0;
}

int32_t sf_scenario_object_part_green(
    const SfScenarioObject *object, uint8_t part, bool hovered) {
  return object && part < SF_MCT_PERSON_PART_LIMIT
    ? sf_scenario_object_part_strength(
        object->part_green_strength[part], object->green_strength, hovered) : 0;
}

int32_t sf_scenario_object_part_blue(
    const SfScenarioObject *object, uint8_t part, bool hovered) {
  return object && part < SF_MCT_PERSON_PART_LIMIT
    ? sf_scenario_object_part_strength(
        object->part_blue_strength[part], object->blue_strength, hovered) : 0;
}
