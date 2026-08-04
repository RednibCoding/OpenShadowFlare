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

#include "assets/scenario_enemy_assets.h"

#include "assets/retail_paths.h"
#include "core/coordinates.h"

#include <stdio.h>
#include <string.h>

typedef struct SfScenarioEnemyRequest {
  int32_t resource_id;
  uint8_t enabled_parts;
  uint8_t directions;
} SfScenarioEnemyRequest;

#define SF_SCENARIO_ENEMY_PRELOAD_WIDTH 1280
#define SF_SCENARIO_ENEMY_PRELOAD_HEIGHT 960

static bool sf_scenario_enemy_path(
    char *path, size_t capacity, const char *root,
    const char *format, int32_t resource_id) {
  char relative[SF_RETAIL_PATH_CAPACITY];
  const int length = snprintf(
    relative, sizeof(relative), format, resource_id);
  return length >= 0 && (size_t) length < sizeof(relative) &&
    sf_retail_path_join(path, capacity, root, relative);
}

static int sf_scenario_enemy_request(
    SfScenarioEnemyRequest *requests, uint16_t count, int32_t resource_id) {
  uint16_t index;
  for (index = 0u; index < count; ++index) {
    if (requests[index].resource_id == resource_id) return (int) index;
  }
  return -1;
}

static bool sf_scenario_enemy_requests(
    const SfMctScenario *scenario, SfScenarioEnemyRequest *requests,
    SfWorldPoint focus, uint16_t *request_count) {
  const SfScreenPoint focus_screen = sf_world_to_screen(focus);
  uint16_t enemy_index;
  *request_count = 0u;
  for (enemy_index = 0u; enemy_index < scenario->enemy_count;
       ++enemy_index) {
    const SfMctEnemy *enemy = &scenario->enemies[enemy_index];
    int request;
    uint8_t enabled = 0u;
    uint8_t part;
    const SfScreenPoint enemy_screen = sf_world_to_screen(
      (SfWorldPoint) {enemy->world_x, enemy->world_y});
    if (enemy->resource_id < 0) continue;
    if (enemy_screen.x < focus_screen.x - SF_SCENARIO_ENEMY_PRELOAD_WIDTH ||
        enemy_screen.x > focus_screen.x + SF_SCENARIO_ENEMY_PRELOAD_WIDTH ||
        enemy_screen.y < focus_screen.y - SF_SCENARIO_ENEMY_PRELOAD_HEIGHT ||
        enemy_screen.y > focus_screen.y + SF_SCENARIO_ENEMY_PRELOAD_HEIGHT)
      continue;
    request = sf_scenario_enemy_request(
      requests, *request_count, enemy->resource_id);
    for (part = 0u; part < SF_MCT_PERSON_PART_LIMIT; ++part) {
      if (!enemy->custom_parts || enemy->part_visibility[part] != 0u)
        enabled = (uint8_t) (enabled | (uint8_t) (1u << part));
    }
    if (request < 0) {
      if (*request_count >= SF_MCT_ENEMY_LIMIT) return false;
      request = (int) (*request_count)++;
      requests[request].resource_id = enemy->resource_id;
    }
    requests[request].enabled_parts = (uint8_t) (
      requests[request].enabled_parts | enabled);
    requests[request].directions = (uint8_t) (
      requests[request].directions |
      (uint8_t) (1u << (uint8_t) enemy->direction));
  }
  return true;
}

static bool sf_scenario_enemy_add_pattern(
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

static bool sf_scenario_enemy_visual_load(
    SfScenarioEnemyVisual *visual, const char *data_root,
    SfScenarioEnemyRequest request, SfArena *arena) {
  SfCafAnimationSelection selections[
    SF_SCENARIO_ENEMY_ANIMATION_COUNT * SF_SCENARIO_ENEMY_DIRECTION_COUNT];
  SfCafSelectedAnimation loaded[
    SF_SCENARIO_ENEMY_ANIMATION_COUNT * SF_SCENARIO_ENEMY_DIRECTION_COUNT];
  uint8_t selected_parts[SF_MCT_PERSON_PART_LIMIT];
  int32_t artwork_patterns[SF_NJP_SPARSE_PATTERN_LIMIT];
  int32_t shadow_patterns[SF_NJP_SPARSE_PATTERN_LIMIT];
  uint16_t artwork_count = 0u;
  uint16_t shadow_count = 0u;
  char path[SF_RETAIL_PATH_CAPACITY];
  uint8_t available_parts = UINT8_MAX;
  uint8_t selected_count = 0u;
  uint8_t direction;
  uint8_t selection_count = 0u;
  uint8_t animation;
  uint8_t part;
  memset(visual, 0, sizeof(*visual));
  memset(loaded, 0, sizeof(loaded));
  visual->resource_id = request.resource_id;
  if (!sf_scenario_enemy_path(
        path, sizeof(path), data_root,
        sf_retail_enemy_paths.animation_format, request.resource_id))
    return false;
  for (animation = 0u; animation < SF_SCENARIO_ENEMY_ANIMATION_COUNT;
       ++animation) {
    for (direction = 0u; direction < SF_SCENARIO_ENEMY_DIRECTION_COUNT;
         ++direction) {
      uint8_t count;
      if (animation == 0u &&
          (request.directions & (uint8_t) (1u << direction)) == 0u) continue;
      if (!sf_caf_chart_direction_part_count(
            path, animation, direction, &count)) return false;
      if (count < available_parts) available_parts = count;
      selections[selection_count].chart = animation;
      selections[selection_count].direction = direction;
      ++selection_count;
    }
  }
  if (available_parts > SF_MCT_PERSON_PART_LIMIT)
    available_parts = SF_MCT_PERSON_PART_LIMIT;
  for (part = 0u; part < available_parts; ++part) {
    if ((request.enabled_parts & (uint8_t) (1u << part)) != 0u)
      selected_parts[selected_count++] = part;
  }
  if (selected_count == 0u || selection_count == 0u ||
      !sf_caf_load_selected_animations(
        path, selections, selection_count,
        selected_parts, selected_count, arena, loaded))
    return false;
  for (direction = 0u; direction < selection_count; ++direction) {
    visual->animations[selections[direction].chart]
      [selections[direction].direction] = loaded[direction];
  }
  for (animation = 0u; animation < SF_SCENARIO_ENEMY_ANIMATION_COUNT;
       ++animation) {
    for (direction = 0u; direction < SF_SCENARIO_ENEMY_DIRECTION_COUNT;
         ++direction) {
      const SfCafSelectedAnimation *selected =
        &visual->animations[animation][direction];
      if (animation == 0u &&
          (request.directions & (uint8_t) (1u << direction)) == 0u) continue;
      for (part = 0u; part < selected->part_count; ++part) {
        uint16_t frame;
        visual->selected_parts = (uint8_t) (
          visual->selected_parts |
          (uint8_t) (1u << selected->parts[part].source_index));
        for (frame = 0u; frame < selected->frame_count; ++frame) {
          const SfCafCell *cell = &selected->parts[part].cells[frame];
          if (((cell->status & 8) == 0 && !sf_scenario_enemy_add_pattern(
                artwork_patterns, &artwork_count, cell->pattern)) ||
              ((cell->status & 8) != 0 && !sf_scenario_enemy_add_pattern(
                shadow_patterns, &shadow_count, cell->pattern))) return false;
        }
      }
    }
  }
  if (artwork_count == 0u || shadow_count == 0u) return false;
  if (!sf_scenario_enemy_path(
        path, sizeof(path), data_root,
        sf_retail_enemy_paths.artwork_format, request.resource_id) ||
      !sf_njp_load_sparse_patterns(
        path, artwork_patterns, artwork_count, arena, &visual->artwork))
    return false;
  if (!sf_scenario_enemy_path(
        path, sizeof(path), data_root,
        sf_retail_enemy_paths.shadow_format, request.resource_id) ||
      !sf_njp_load_sparse_patterns(
        path, shadow_patterns, shadow_count, arena, &visual->shadows))
    return false;
  return true;
}


bool sf_scenario_enemy_assets_load(
    SfScenarioEnemyAssets *assets, const char *data_root,
    const SfMctScenario *scenario, SfWorldPoint focus, SfArena *arena) {
  SfScenarioEnemyRequest requests[SF_MCT_ENEMY_LIMIT];
  uint16_t request_count;
  uint16_t index;
  size_t mark;
  bool success = false;
  if (!assets || !data_root || !scenario || !arena) return false;
  mark = sf_arena_mark(arena);
  memset(assets, 0, sizeof(*assets));
  memset(requests, 0, sizeof(requests));
  if (!sf_scenario_enemy_requests(
        scenario, requests, focus, &request_count)) goto done;
  if (request_count > 0u) {
    assets->visuals = (SfScenarioEnemyVisual *) sf_arena_push_zero(
      arena, (size_t) request_count * sizeof(*assets->visuals), sizeof(void *));
    if (!assets->visuals) goto done;
    for (index = 0u; index < request_count; ++index) {
      if (!sf_scenario_enemy_visual_load(
            &assets->visuals[index], data_root, requests[index], arena))
        goto done;
    }
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

const SfScenarioEnemyVisual *sf_scenario_enemy_visual(
    const SfScenarioEnemyAssets *assets, int32_t resource_id) {
  uint16_t index;
  if (!assets) return NULL;
  for (index = 0u; index < assets->visual_count; ++index) {
    if (assets->visuals[index].resource_id == resource_id)
      return &assets->visuals[index];
  }
  return NULL;
}

bool sf_scenario_enemy_frame_assets(
    const SfScenarioEnemyAssets *assets, int32_t resource_id,
    uint16_t chart, uint8_t direction, SfScenarioEnemyFrameAssets *frame) {
  const SfScenarioEnemyVisual *visual;
  if (!assets || !frame || direction >= SF_SCENARIO_ENEMY_DIRECTION_COUNT)
    return false;
  memset(frame, 0, sizeof(*frame));
  if (assets->attack.loaded && assets->attack.resource_id == resource_id &&
      assets->attack.chart == chart) {
    frame->animation = &assets->attack.animations[direction];
    frame->artwork = &assets->attack.artwork;
    frame->shadows = &assets->attack.shadows;
    return frame->animation->frame_count > 0u;
  }
  if (chart >= SF_SCENARIO_ENEMY_ANIMATION_COUNT) return false;
  visual = sf_scenario_enemy_visual(assets, resource_id);
  if (!visual) return false;
  frame->animation = &visual->animations[chart][direction];
  frame->artwork = &visual->artwork;
  frame->shadows = &visual->shadows;
  return frame->animation->frame_count > 0u;
}
