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

#include "assets/scenario_object_assets.h"

#include "assets/retail_paths.h"

#include <stdio.h>
#include <string.h>

typedef struct SfScenarioObjectResourceRequest {
  int32_t resource_id;
  uint8_t enabled_parts;
} SfScenarioObjectResourceRequest;

typedef struct SfScenarioObjectStaticRequest {
  int32_t pattern;
  uint8_t resource;
} SfScenarioObjectStaticRequest;

typedef struct SfScenarioObjectAnimationRequest {
  uint16_t chart;
  uint8_t direction;
  uint8_t resource;
} SfScenarioObjectAnimationRequest;

static bool sf_scenario_object_path(
    char *path, size_t capacity, const char *root,
    const char *format, int32_t resource_id) {
  char relative[SF_RETAIL_PATH_CAPACITY];
  const int length = snprintf(relative, sizeof(relative), format, resource_id);
  return length >= 0 && (size_t) length < sizeof(relative) &&
    sf_retail_path_join(path, capacity, root, relative);
}

static bool sf_scenario_object_file_exists(const char *path) {
  FILE *file = fopen(path, "rb");
  if (!file) return false;
  fclose(file);
  return true;
}

static bool sf_scenario_object_existing_path(
    char *path, size_t capacity, const char *root,
    const char *preferred, const char *alternate, int32_t resource_id) {
  if (sf_scenario_object_path(
        path, capacity, root, preferred, resource_id) &&
      sf_scenario_object_file_exists(path)) return true;
  return alternate && sf_scenario_object_path(
      path, capacity, root, alternate, resource_id) &&
    sf_scenario_object_file_exists(path);
}

static int sf_scenario_object_resource_request(
    const SfScenarioObjectResourceRequest *requests,
    uint8_t count, int32_t resource_id) {
  uint8_t index;
  for (index = 0u; index < count; ++index)
    if (requests[index].resource_id == resource_id) return (int) index;
  return -1;
}

static bool sf_scenario_object_has_static_request(
    const SfScenarioObjectStaticRequest *requests, uint8_t count,
    uint8_t resource, int32_t pattern) {
  uint8_t index;
  for (index = 0u; index < count; ++index)
    if (requests[index].resource == resource &&
        requests[index].pattern == pattern) return true;
  return false;
}

static bool sf_scenario_object_has_animation_request(
    const SfScenarioObjectAnimationRequest *requests, uint8_t count,
    uint8_t resource, uint16_t chart, uint8_t direction) {
  uint8_t index;
  for (index = 0u; index < count; ++index)
    if (requests[index].resource == resource &&
        requests[index].chart == chart &&
        requests[index].direction == direction) return true;
  return false;
}

static bool sf_scenario_object_requests(
    const SfMctScenario *scenario,
    SfScenarioObjectResourceRequest *resources, uint8_t *resource_count,
    SfScenarioObjectStaticRequest *statics, uint8_t *static_count,
    SfScenarioObjectAnimationRequest *animations, uint8_t *animation_count) {
  uint8_t object_index;
  *resource_count = 0u;
  *static_count = 0u;
  *animation_count = 0u;
  for (object_index = 0u; object_index < scenario->object_count;
       ++object_index) {
    const SfMctObject *object = &scenario->objects[object_index];
    int resource;
    uint8_t part;
    uint8_t enabled = 0u;
    if (object->resource_id < 0) continue;
    resource = sf_scenario_object_resource_request(
      resources, *resource_count, object->resource_id);
    if (resource < 0) {
      if (*resource_count >= SF_MCT_OBJECT_LIMIT) return false;
      resource = (*resource_count)++;
      resources[resource].resource_id = object->resource_id;
    }
    for (part = 0u; part < SF_MCT_PERSON_PART_LIMIT; ++part)
      if (!object->custom_parts || object->part_visibility[part] != 0u)
        enabled = (uint8_t) (enabled | (uint8_t) (1u << part));
    resources[resource].enabled_parts = (uint8_t) (
      resources[resource].enabled_parts | enabled);
    if (object->visual_mode != 0 && object->static_pattern >= 0) {
      if (!sf_scenario_object_has_static_request(
            statics, *static_count, (uint8_t) resource,
            object->static_pattern)) {
        if (*static_count >= SF_MCT_OBJECT_LIMIT) return false;
        statics[*static_count] = (SfScenarioObjectStaticRequest) {
          object->static_pattern, (uint8_t) resource};
        ++*static_count;
      }
    } else if (object->visual_mode == 0 && object->animation_chart >= 0) {
      if (object->animation_chart > UINT16_MAX || object->direction < 0 ||
          object->direction > 8) return false;
      if (!sf_scenario_object_has_animation_request(
            animations, *animation_count, (uint8_t) resource,
            (uint16_t) object->animation_chart, (uint8_t) object->direction)) {
        if (*animation_count >= SF_MCT_OBJECT_LIMIT) return false;
        animations[*animation_count] = (SfScenarioObjectAnimationRequest) {
          (uint16_t) object->animation_chart,
          (uint8_t) object->direction, (uint8_t) resource};
        ++*animation_count;
      }
    }
  }
  return true;
}

static bool sf_scenario_object_add_pattern(
    int32_t *patterns, uint16_t *count, int32_t pattern) {
  uint16_t index;
  if (pattern < 0) return true;
  for (index = 0u; index < *count; ++index)
    if (patterns[index] == pattern) return true;
  if (*count >= SF_NJP_SPARSE_PATTERN_LIMIT) return false;
  patterns[(*count)++] = pattern;
  return true;
}

static bool sf_scenario_object_load_static(
    SfScenarioObjectVisual *visual, const char *data_root,
    const SfScenarioObjectStaticRequest *requests, uint8_t request_count,
    uint8_t resource, SfArena *arena) {
  uint8_t patterns[SF_NJP_DECODED_PATTERN_LIMIT];
  uint8_t count = 0u;
  uint8_t shadow_count = 0u;
  uint8_t available_shadow_count = 0u;
  uint8_t index;
  SfNjpPatternBounds shadow_bounds[SF_NJP_PATTERN_FILE_LIMIT];
  char path[SF_RETAIL_PATH_CAPACITY];
  for (index = 0u; index < request_count; ++index) {
    if (requests[index].resource == resource) {
      if (requests[index].pattern < 0 || requests[index].pattern > UINT8_MAX ||
          count >= SF_NJP_DECODED_PATTERN_LIMIT) return false;
      patterns[count++] = (uint8_t) requests[index].pattern;
    }
  }
  if (count == 0u) return true;
  if (!sf_scenario_object_existing_path(
        path, sizeof(path), data_root,
        sf_retail_object_paths.static_artwork_format,
        sf_retail_object_paths.static_artwork_alternate_format,
        visual->resource_id) ||
      !sf_njp_load_decoded_patterns(
        path, patterns, count, arena, &visual->static_artwork)) return false;
  if (!sf_scenario_object_existing_path(
        path, sizeof(path), data_root,
        sf_retail_object_paths.static_shadow_format,
        sf_retail_object_paths.static_shadow_alternate_format,
        visual->resource_id)) return true;
  if (!sf_njp_read_pattern_bounds(
        path, shadow_bounds, SF_NJP_PATTERN_FILE_LIMIT,
        &available_shadow_count)) return false;
  for (index = 0u; index < count; ++index)
    if (patterns[index] < available_shadow_count)
      patterns[shadow_count++] = patterns[index];
  if (shadow_count == 0u) return true;
  return sf_njp_load_decoded_patterns(
    path, patterns, shadow_count, arena, &visual->static_shadows);
}

static bool sf_scenario_object_load_animation(
    SfScenarioObjectVisual *visual, const char *data_root,
    const SfScenarioObjectResourceRequest *resource_request,
    const SfScenarioObjectAnimationRequest *requests, uint8_t request_count,
    uint8_t resource, SfArena *arena) {
  SfCafAnimationSelection selections[SF_MCT_OBJECT_LIMIT];
  uint8_t selected_parts[SF_MCT_PERSON_PART_LIMIT];
  int32_t patterns[SF_NJP_SPARSE_PATTERN_LIMIT];
  uint16_t pattern_count = 0u;
  uint8_t available_parts = UINT8_MAX;
  uint8_t selection_count = 0u;
  uint8_t selected_part_count = 0u;
  uint8_t index;
  uint8_t part;
  char path[SF_RETAIL_PATH_CAPACITY];
  for (index = 0u; index < request_count; ++index) {
    if (requests[index].resource != resource) continue;
    selections[selection_count] = (SfCafAnimationSelection) {
      requests[index].chart, requests[index].direction};
    ++selection_count;
  }
  if (selection_count == 0u) return true;
  if (!sf_scenario_object_path(
        path, sizeof(path), data_root,
        sf_retail_object_paths.animation_format, visual->resource_id))
    return false;
  for (index = 0u; index < selection_count; ++index) {
    uint8_t count;
    if (!sf_caf_chart_direction_part_count(
          path, selections[index].chart, selections[index].direction,
          &count)) return false;
    if (count < available_parts) available_parts = count;
  }
  if (available_parts > SF_MCT_PERSON_PART_LIMIT)
    available_parts = SF_MCT_PERSON_PART_LIMIT;
  for (part = 0u; part < available_parts; ++part)
    if ((resource_request->enabled_parts & (uint8_t) (1u << part)) != 0u)
      selected_parts[selected_part_count++] = part;
  if (selected_part_count == 0u) return false;
  visual->animations = (SfCafSelectedAnimation *) sf_arena_push_zero(
    arena, (size_t) selection_count * sizeof(*visual->animations),
    sizeof(void *));
  visual->animation_keys = (SfScenarioObjectAnimationKey *) sf_arena_push_zero(
    arena, (size_t) selection_count * sizeof(*visual->animation_keys),
    sizeof(uint16_t));
  if (!visual->animations || !visual->animation_keys ||
      !sf_caf_load_selected_animations(
        path, selections, selection_count,
        selected_parts, selected_part_count, arena, visual->animations))
    return false;
  visual->animation_count = selection_count;
  for (index = 0u; index < selection_count; ++index) {
    const SfCafSelectedAnimation *animation = &visual->animations[index];
    visual->animation_keys[index].chart = selections[index].chart;
    visual->animation_keys[index].direction = selections[index].direction;
    for (part = 0u; part < animation->part_count; ++part) {
      uint16_t frame;
      for (frame = 0u; frame < animation->frame_count; ++frame)
        if (!sf_scenario_object_add_pattern(
              patterns, &pattern_count,
              animation->parts[part].cells[frame].pattern)) return false;
    }
  }
  if (pattern_count == 0u || !sf_scenario_object_path(
        path, sizeof(path), data_root,
        sf_retail_object_paths.animation_artwork_format,
        visual->resource_id)) return false;
  return sf_njp_load_sparse_patterns(
    path, patterns, pattern_count, arena, &visual->animation_artwork);
}

bool sf_scenario_object_assets_load(
    SfScenarioObjectAssets *assets, const char *data_root,
    const SfMctScenario *scenario, SfArena *arena) {
  SfScenarioObjectResourceRequest resources[SF_MCT_OBJECT_LIMIT];
  SfScenarioObjectStaticRequest statics[SF_MCT_OBJECT_LIMIT];
  SfScenarioObjectAnimationRequest animations[SF_MCT_OBJECT_LIMIT];
  uint8_t resource_count;
  uint8_t static_count;
  uint8_t animation_count;
  uint8_t index;
  size_t mark;
  bool success = false;
  if (!assets || !data_root || !scenario || !arena) return false;
  mark = sf_arena_mark(arena);
  memset(assets, 0, sizeof(*assets));
  memset(resources, 0, sizeof(resources));
  if (!sf_scenario_object_requests(
        scenario, resources, &resource_count,
        statics, &static_count, animations, &animation_count)) goto done;
  if (resource_count > 0u) {
    assets->visuals = (SfScenarioObjectVisual *) sf_arena_push_zero(
      arena, (size_t) resource_count * sizeof(*assets->visuals),
      sizeof(void *));
    if (!assets->visuals) goto done;
  }
  for (index = 0u; index < resource_count; ++index) {
    SfScenarioObjectVisual *visual = &assets->visuals[index];
    visual->resource_id = resources[index].resource_id;
    if (!sf_scenario_object_load_static(
          visual, data_root, statics, static_count, index, arena) ||
        !sf_scenario_object_load_animation(
          visual, data_root, &resources[index],
          animations, animation_count, index, arena)) goto done;
  }
  assets->visual_count = resource_count;
  assets->memory_bytes = sf_arena_mark(arena) - mark;
  success = true;
done:
  if (!success) {
    (void) sf_arena_rewind(arena, mark);
    memset(assets, 0, sizeof(*assets));
  }
  return success;
}

const SfScenarioObjectVisual *sf_scenario_object_visual(
    const SfScenarioObjectAssets *assets, int32_t resource_id) {
  uint8_t index;
  if (!assets) return NULL;
  for (index = 0u; index < assets->visual_count; ++index)
    if (assets->visuals[index].resource_id == resource_id)
      return &assets->visuals[index];
  return NULL;
}

const SfCafSelectedAnimation *sf_scenario_object_animation(
    const SfScenarioObjectVisual *visual, int32_t chart, uint8_t direction) {
  uint8_t index;
  if (!visual || chart < 0 || chart > UINT16_MAX) return NULL;
  for (index = 0u; index < visual->animation_count; ++index)
    if (visual->animation_keys[index].chart == (uint16_t) chart &&
        visual->animation_keys[index].direction == direction)
      return &visual->animations[index];
  return NULL;
}
