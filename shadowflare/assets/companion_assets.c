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

#include "assets/companion_assets.h"

#include "assets/retail_paths.h"

#include <stdio.h>
#include <string.h>

static bool sf_companion_asset_path(
    char *path, size_t capacity, const char *root,
    const char *format, int32_t resource_id) {
  char relative[SF_RETAIL_PATH_CAPACITY];
  const int length = snprintf(relative, sizeof(relative), format, resource_id);
  return length >= 0 && (size_t) length < sizeof(relative) &&
    sf_retail_path_join(path, capacity, root, relative);
}

static bool sf_companion_add_pattern(
    int32_t *patterns, uint16_t *count, int32_t pattern) {
  uint16_t index;
  if (pattern < 0) return true;
  for (index = 0u; index < *count; ++index)
    if (patterns[index] == pattern) return true;
  if (*count >= SF_NJP_SPARSE_PATTERN_LIMIT) return false;
  patterns[(*count)++] = pattern;
  return true;
}

bool sf_companion_assets_load(
    SfCompanionAssets *assets, const char *data_root,
    const SfCompanionProfile *profile, SfArena *arena) {
  SfCafAnimationSelection selections[
    SF_COMPANION_ANIMATION_COUNT * SF_COMPANION_DIRECTION_COUNT];
  int32_t artwork_patterns[SF_NJP_SPARSE_PATTERN_LIMIT];
  int32_t shadow_patterns[SF_NJP_SPARSE_PATTERN_LIMIT];
  uint8_t selected_parts[SF_COMPANION_PART_LIMIT];
  uint16_t artwork_count = 0u;
  uint16_t shadow_count = 0u;
  uint8_t available_parts = UINT8_MAX;
  uint8_t chart;
  uint8_t direction;
  uint8_t part;
  char path[SF_RETAIL_PATH_CAPACITY];
  size_t mark;
  bool success = false;
  if (!assets || !data_root || !profile || profile->resource_id < 0 || !arena)
    return false;
  mark = sf_arena_mark(arena);
  memset(assets, 0, sizeof(*assets));
  assets->resource_id = profile->resource_id;
  if (!sf_companion_asset_path(
        path, sizeof(path), data_root,
        sf_retail_companion_paths.animation_format, profile->resource_id))
    goto done;
  for (chart = 0u; chart < SF_COMPANION_ANIMATION_COUNT; ++chart) {
    for (direction = 0u; direction < SF_COMPANION_DIRECTION_COUNT;
         ++direction) {
      const uint8_t index = (uint8_t) (
        chart * SF_COMPANION_DIRECTION_COUNT + direction);
      uint8_t count;
      if (!sf_caf_chart_direction_part_count(path, chart, direction, &count))
        goto done;
      if (count < available_parts) available_parts = count;
      selections[index].chart = chart;
      selections[index].direction = direction;
    }
  }
  if (available_parts > SF_COMPANION_PART_LIMIT)
    available_parts = SF_COMPANION_PART_LIMIT;
  if (available_parts == 0u) goto done;
  for (part = 0u; part < available_parts; ++part) {
    selected_parts[part] = part;
    assets->selected_parts = (uint8_t) (
      assets->selected_parts | (uint8_t) (1u << part));
  }
  if (!sf_caf_load_selected_animations(
        path, selections,
        SF_COMPANION_ANIMATION_COUNT * SF_COMPANION_DIRECTION_COUNT,
        selected_parts, available_parts, arena, &assets->animations[0][0]))
    goto done;
  for (chart = 0u; chart < SF_COMPANION_ANIMATION_COUNT; ++chart) {
    for (direction = 0u; direction < SF_COMPANION_DIRECTION_COUNT;
         ++direction) {
      const SfCafSelectedAnimation *animation =
        &assets->animations[chart][direction];
      for (part = 0u; part < animation->part_count; ++part) {
        uint16_t frame;
        for (frame = 0u; frame < animation->frame_count; ++frame) {
          const SfCafCell *cell = &animation->parts[part].cells[frame];
          if (((cell->status & 8) == 0 && !sf_companion_add_pattern(
                artwork_patterns, &artwork_count, cell->pattern)) ||
              ((cell->status & 8) != 0 && !sf_companion_add_pattern(
                shadow_patterns, &shadow_count, cell->pattern))) goto done;
        }
      }
    }
  }
  if (artwork_count == 0u || shadow_count == 0u ||
      !sf_companion_asset_path(
        path, sizeof(path), data_root,
        sf_retail_companion_paths.artwork_format, profile->resource_id) ||
      !sf_njp_load_sparse_patterns(
        path, artwork_patterns, artwork_count, arena, &assets->artwork) ||
      !sf_companion_asset_path(
        path, sizeof(path), data_root,
        sf_retail_companion_paths.shadow_format, profile->resource_id) ||
      !sf_njp_load_sparse_patterns(
        path, shadow_patterns, shadow_count, arena, &assets->shadows))
    goto done;
  assets->memory_bytes = sf_arena_mark(arena) - mark;
  success = true;
done:
  if (!success) {
    (void) sf_arena_rewind(arena, mark);
    memset(assets, 0, sizeof(*assets));
  }
  return success;
}
