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

#include <stdio.h>
#include <string.h>

static bool sf_enemy_attack_path(
    char *path, size_t capacity, const char *root,
    const char *format, int32_t resource_id) {
  char relative[SF_RETAIL_PATH_CAPACITY];
  const int length = snprintf(
    relative, sizeof(relative), format, resource_id);
  return length >= 0 && (size_t) length < sizeof(relative) &&
    sf_retail_path_join(path, capacity, root, relative);
}

static bool sf_enemy_attack_add_pattern(
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

static bool sf_enemy_attack_request(
    const SfMctScenario *scenario, int32_t resource_id, uint16_t chart,
    uint8_t *enabled_parts) {
  uint16_t enemy_index;
  bool found = false;
  for (enemy_index = 0u; enemy_index < scenario->enemy_count; ++enemy_index) {
    const SfMctEnemy *enemy = &scenario->enemies[enemy_index];
    uint8_t variant;
    uint8_t part;
    if (enemy->resource_id != resource_id) continue;
    for (variant = 0u; variant < 3u; ++variant) {
      if (enemy->post_ai_values[41u + variant] + 4 == chart) break;
    }
    if (variant == 3u) continue;
    found = true;
    for (part = 0u; part < SF_MCT_PERSON_PART_LIMIT; ++part) {
      if (!enemy->custom_parts || enemy->part_visibility[part] != 0u)
        *enabled_parts = (uint8_t) (
          *enabled_parts | (uint8_t) (1u << part));
    }
  }
  return found;
}

static bool sf_enemy_attack_collect_patterns(
    const SfCafSelectedAnimation *animations,
    int32_t *artwork_patterns, uint16_t *artwork_count,
    int32_t *shadow_patterns, uint16_t *shadow_count) {
  uint8_t direction;
  for (direction = 0u; direction < SF_SCENARIO_ENEMY_DIRECTION_COUNT;
       ++direction) {
    const SfCafSelectedAnimation *animation = &animations[direction];
    uint8_t part;
    for (part = 0u; part < animation->part_count; ++part) {
      uint16_t frame;
      for (frame = 0u; frame < animation->frame_count; ++frame) {
        const SfCafCell *cell = &animation->parts[part].cells[frame];
        if (((cell->status & 8) == 0 && !sf_enemy_attack_add_pattern(
              artwork_patterns, artwork_count, cell->pattern)) ||
            ((cell->status & 8) != 0 && !sf_enemy_attack_add_pattern(
              shadow_patterns, shadow_count, cell->pattern))) return false;
      }
    }
  }
  return *artwork_count > 0u && *shadow_count > 0u;
}

bool sf_scenario_enemy_attack_assets_load(
    SfScenarioEnemyAttackAssets *attack, const char *data_root,
    const SfMctScenario *scenario, int32_t resource_id, uint16_t chart,
    SfArena *arena) {
  SfCafAnimationSelection selections[SF_SCENARIO_ENEMY_DIRECTION_COUNT];
  uint8_t selected_parts[SF_MCT_PERSON_PART_LIMIT];
  int32_t artwork_patterns[SF_NJP_SPARSE_PATTERN_LIMIT];
  int32_t shadow_patterns[SF_NJP_SPARSE_PATTERN_LIMIT];
  uint16_t artwork_count = 0u;
  uint16_t shadow_count = 0u;
  uint8_t selected_count = 0u;
  uint8_t enabled_parts = 0u;
  uint8_t available_parts = UINT8_MAX;
  uint8_t direction;
  uint8_t part;
  char path[SF_RETAIL_PATH_CAPACITY];
  size_t mark;
  bool success = false;
  if (!attack || !data_root || !scenario || resource_id < 0 || !arena)
    return false;
  mark = sf_arena_mark(arena);
  memset(attack, 0, sizeof(*attack));
  if (chart > UINT8_MAX || !sf_enemy_attack_request(
        scenario, resource_id, chart, &enabled_parts) ||
      !sf_enemy_attack_path(
        path, sizeof(path), data_root,
        sf_retail_enemy_paths.animation_format, resource_id)) goto done;
  for (direction = 0u; direction < SF_SCENARIO_ENEMY_DIRECTION_COUNT;
       ++direction) {
    uint8_t count;
    if (!sf_caf_chart_direction_part_count(
          path, chart, direction, &count)) goto done;
    if (count < available_parts) available_parts = count;
    selections[direction].chart = chart;
    selections[direction].direction = direction;
  }
  if (available_parts > SF_MCT_PERSON_PART_LIMIT)
    available_parts = SF_MCT_PERSON_PART_LIMIT;
  for (part = 0u; part < available_parts; ++part) {
    if ((enabled_parts & (uint8_t) (1u << part)) != 0u)
      selected_parts[selected_count++] = part;
  }
  if (selected_count == 0u || !sf_caf_load_selected_animations(
        path, selections, SF_SCENARIO_ENEMY_DIRECTION_COUNT,
        selected_parts, selected_count, arena, attack->animations) ||
      !sf_enemy_attack_collect_patterns(
        attack->animations, artwork_patterns, &artwork_count,
        shadow_patterns, &shadow_count) ||
      !sf_enemy_attack_path(
        path, sizeof(path), data_root,
        sf_retail_enemy_paths.artwork_format, resource_id) ||
      !sf_njp_load_sparse_patterns(
        path, artwork_patterns, artwork_count, arena, &attack->artwork) ||
      !sf_enemy_attack_path(
        path, sizeof(path), data_root,
        sf_retail_enemy_paths.shadow_format, resource_id) ||
      !sf_njp_load_sparse_patterns(
        path, shadow_patterns, shadow_count, arena, &attack->shadows))
    goto done;
  attack->resource_id = resource_id;
  attack->chart = chart;
  attack->memory_bytes = sf_arena_mark(arena) - mark;
  attack->loaded = true;
  success = true;
done:
  if (!success) {
    (void) sf_arena_rewind(arena, mark);
    memset(attack, 0, sizeof(*attack));
  }
  return success;
}
