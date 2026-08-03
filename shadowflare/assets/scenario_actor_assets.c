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

#include "assets/scenario_actor_assets.h"

#include "assets/retail_paths.h"

#include <stdio.h>
#include <string.h>

typedef struct SfScenarioActorRequest {
  int32_t resource_id;
  uint8_t enabled_parts;
  uint8_t animation_count;
} SfScenarioActorRequest;

static bool sf_scenario_actor_path(
    char *path, size_t capacity, const char *root,
    const char *format, int32_t resource_id) {
  char relative[SF_RETAIL_PATH_CAPACITY];
  const int length = snprintf(
    relative, sizeof(relative), format, resource_id);
  return length >= 0 && (size_t) length < sizeof(relative) &&
    sf_retail_path_join(path, capacity, root, relative);
}

static int sf_scenario_actor_request(
    SfScenarioActorRequest *requests, uint8_t count, int32_t resource_id) {
  uint8_t index;
  for (index = 0u; index < count; ++index) {
    if (requests[index].resource_id == resource_id) return (int) index;
  }
  return -1;
}

static bool sf_scenario_actor_requests(
    const SfMctScenario *scenario, SfScenarioActorRequest *requests,
    uint8_t *request_count) {
  uint8_t person_index;
  *request_count = 0u;
  for (person_index = 0u; person_index < scenario->people_count;
       ++person_index) {
    const SfMctPerson *person = &scenario->people[person_index];
    int request = sf_scenario_actor_request(
      requests, *request_count, person->resource_id);
    uint8_t enabled = 0u;
    uint8_t part;
    if (person->resource_id < 0) return false;
    for (part = 0u; part < SF_MCT_PERSON_PART_LIMIT; ++part) {
      if (!person->custom_parts || person->part_visibility[part] != 0u)
        enabled = (uint8_t) (enabled | (uint8_t) (1u << part));
    }
    if (request < 0) {
      if (*request_count >= SF_MCT_PERSON_LIMIT) return false;
      request = (*request_count)++;
      requests[request].resource_id = person->resource_id;
    }
    requests[request].enabled_parts = (uint8_t) (
      requests[request].enabled_parts | enabled);
    if (person->wandering_enabled && person->walk_speed > 0 &&
        person->walk_duration > 0)
      requests[request].animation_count = SF_SCENARIO_ACTOR_ANIMATION_COUNT;
    else if (requests[request].animation_count == 0u)
      requests[request].animation_count = 1u;
  }
  return *request_count > 0u;
}

static bool sf_scenario_actor_add_pattern(
    int32_t *patterns, uint16_t *count, int32_t pattern) {
  uint16_t index;
  if (pattern < 0) return true;
  for (index = 0u; index < *count; ++index) {
    if (patterns[index] == pattern) return true;
  }
  if (*count >= SF_NJP_SPARSE_PATTERN_LIMIT) return false;
  patterns[(*count)++] = pattern;
  return true;
}

static bool sf_scenario_actor_visual_load(
    SfScenarioActorVisual *visual, const char *data_root,
    SfScenarioActorRequest request, SfArena *arena) {
  SfCafAnimationSelection selections[
    SF_SCENARIO_ACTOR_ANIMATION_COUNT * SF_SCENARIO_ACTOR_DIRECTION_COUNT];
  uint8_t selected_parts[SF_MCT_PERSON_PART_LIMIT];
  int32_t artwork_patterns[SF_NJP_SPARSE_PATTERN_LIMIT];
  int32_t shadow_patterns[SF_NJP_SPARSE_PATTERN_LIMIT];
  uint16_t artwork_count = 0u;
  uint16_t shadow_count = 0u;
  char path[SF_RETAIL_PATH_CAPACITY];
  uint8_t available_parts = UINT8_MAX;
  uint8_t selected_count = 0u;
  uint8_t animation;
  uint8_t direction;
  uint8_t part;
  memset(visual, 0, sizeof(*visual));
  visual->resource_id = request.resource_id;
  if (!sf_scenario_actor_path(
        path, sizeof(path), data_root,
        sf_retail_people_paths.animation_format, request.resource_id))
    return false;
  for (animation = 0u; animation < request.animation_count; ++animation) {
    for (direction = 0u; direction < SF_SCENARIO_ACTOR_DIRECTION_COUNT;
         ++direction) {
      const uint8_t selection = (uint8_t) (
        animation * SF_SCENARIO_ACTOR_DIRECTION_COUNT + direction);
      uint8_t count;
      if (!sf_caf_chart_direction_part_count(
            path, animation, direction, &count)) return false;
      if (count < available_parts) available_parts = count;
      selections[selection].chart = animation;
      selections[selection].direction = direction;
    }
  }
  if (available_parts > SF_MCT_PERSON_PART_LIMIT)
    available_parts = SF_MCT_PERSON_PART_LIMIT;
  for (part = 0u; part < available_parts; ++part) {
    if ((request.enabled_parts & (uint8_t) (1u << part)) != 0u)
      selected_parts[selected_count++] = part;
  }
  if (selected_count == 0u || !sf_caf_load_selected_animations(
        path, selections,
        (uint8_t) (request.animation_count *
          SF_SCENARIO_ACTOR_DIRECTION_COUNT),
        selected_parts, selected_count, arena, &visual->animations[0][0]))
    return false;
  for (animation = 0u; animation < request.animation_count; ++animation) {
    for (direction = 0u; direction < SF_SCENARIO_ACTOR_DIRECTION_COUNT;
         ++direction) {
      const SfCafSelectedAnimation *selected =
        &visual->animations[animation][direction];
      for (part = 0u; part < selected->part_count; ++part) {
        uint16_t frame;
        visual->selected_parts = (uint8_t) (
          visual->selected_parts |
          (uint8_t) (1u << selected->parts[part].source_index));
        for (frame = 0u; frame < selected->frame_count; ++frame) {
          const SfCafCell *cell = &selected->parts[part].cells[frame];
          if (((cell->status & 8) == 0 && !sf_scenario_actor_add_pattern(
                artwork_patterns, &artwork_count, cell->pattern)) ||
              ((cell->status & 8) != 0 && !sf_scenario_actor_add_pattern(
                shadow_patterns, &shadow_count, cell->pattern))) return false;
        }
      }
    }
  }
  if (artwork_count == 0u || shadow_count == 0u ||
      !sf_scenario_actor_path(
        path, sizeof(path), data_root,
        sf_retail_people_paths.artwork_format, request.resource_id) ||
      !sf_njp_load_sparse_patterns(
        path, artwork_patterns, artwork_count, arena, &visual->artwork) ||
      !sf_scenario_actor_path(
        path, sizeof(path), data_root,
        sf_retail_people_paths.shadow_format, request.resource_id) ||
      !sf_njp_load_sparse_patterns(
        path, shadow_patterns, shadow_count, arena, &visual->shadows))
    return false;
  return true;
}

bool sf_scenario_actor_assets_load(
    SfScenarioActorAssets *assets, const char *data_root,
    const SfMctScenario *scenario, SfArena *arena) {
  SfScenarioActorRequest requests[SF_MCT_PERSON_LIMIT];
  uint8_t request_count;
  uint8_t index;
  size_t mark;
  bool success = false;
  if (!assets || !data_root || !scenario || !arena) return false;
  mark = sf_arena_mark(arena);
  memset(assets, 0, sizeof(*assets));
  memset(requests, 0, sizeof(requests));
  if (!sf_scenario_actor_requests(
        scenario, requests, &request_count)) goto done;
  assets->visuals = (SfScenarioActorVisual *) sf_arena_push_zero(
    arena, (size_t) request_count * sizeof(*assets->visuals), sizeof(void *));
  if (!assets->visuals) goto done;
  for (index = 0u; index < request_count; ++index) {
    if (!sf_scenario_actor_visual_load(
          &assets->visuals[index], data_root, requests[index], arena))
      goto done;
  }
  assets->visual_count = request_count;
  assets->memory_bytes = sf_arena_mark(arena) - mark;
  success = true;
done:
  if (!success) {
    (void) sf_arena_rewind(arena, mark);
    memset(assets, 0, sizeof(*assets));
  }
  return success;
}

const SfScenarioActorVisual *sf_scenario_actor_visual(
    const SfScenarioActorAssets *assets, int32_t resource_id) {
  uint8_t index;
  if (!assets) return NULL;
  for (index = 0u; index < assets->visual_count; ++index) {
    if (assets->visuals[index].resource_id == resource_id)
      return &assets->visuals[index];
  }
  return NULL;
}
