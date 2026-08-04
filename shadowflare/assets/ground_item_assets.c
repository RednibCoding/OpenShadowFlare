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

#include "assets/ground_item_assets.h"

#include "assets/ground_item_requests.h"
#include "assets/retail_paths.h"

#include <stdio.h>
#include <string.h>

#define SF_GROUND_ITEM_PATTERN_LIMIT 512u

static bool sf_ground_item_path(
    char *path, size_t capacity, const char *root,
    const char *format, int32_t resource_id) {
  char relative[SF_RETAIL_PATH_CAPACITY];
  const int length = snprintf(
    relative, sizeof(relative), format, resource_id);
  return length >= 0 && (size_t) length < sizeof(relative) &&
    sf_retail_path_join(path, capacity, root, relative);
}

static bool sf_ground_item_add_value(
    int32_t *values, uint16_t *count, uint16_t capacity, int32_t value) {
  uint16_t index;
  if (value < 0) return true;
  for (index = 0u; index < *count; ++index)
    if (values[index] == value) return true;
  if (*count >= capacity) return false;
  values[(*count)++] = value;
  return true;
}

static bool sf_ground_item_visual_load(
    SfGroundItemVisual *visual, const char *data_root,
    SfGroundItemResourceRequest request, SfArena *arena) {
  SfCafAnimationSelection selections[SF_GROUND_ITEM_DEFINITION_LIMIT];
  uint8_t selected_parts[SF_CAF_SELECTED_PART_LIMIT];
  int32_t artwork_patterns[SF_GROUND_ITEM_PATTERN_LIMIT];
  int32_t shadow_patterns[SF_GROUND_ITEM_PATTERN_LIMIT];
  int32_t palettes[SF_NJP_SPARSE_PALETTE_LIMIT];
  uint16_t artwork_count = 0u;
  uint16_t shadow_count = 0u;
  uint16_t palette_count = 0u;
  uint8_t available_parts = UINT8_MAX;
  uint8_t chart_index;
  uint8_t part;
  char path[SF_RETAIL_PATH_CAPACITY];
  memset(visual, 0, sizeof(*visual));
  visual->resource_id = request.resource_id;
  if (!sf_ground_item_path(
        path, sizeof(path), data_root,
        sf_retail_ground_item_paths.animation_format,
        request.resource_id)) return false;
  for (chart_index = 0u; chart_index < request.chart_count; ++chart_index) {
    uint8_t part_count;
    if (!sf_caf_chart_direction_part_count(
          path, request.charts[chart_index], 8u, &part_count)) return false;
    if (part_count < available_parts) available_parts = part_count;
    selections[chart_index].chart = request.charts[chart_index];
    selections[chart_index].direction = 8u;
  }
  if (available_parts > SF_CAF_SELECTED_PART_LIMIT)
    available_parts = SF_CAF_SELECTED_PART_LIMIT;
  if (available_parts == 0u) return false;
  for (part = 0u; part < available_parts; ++part)
    selected_parts[part] = part;
  visual->animations = (SfCafSelectedAnimation *) sf_arena_push_zero(
    arena, (size_t) request.chart_count * sizeof(*visual->animations),
    sizeof(void *));
  visual->charts = (uint16_t *) sf_arena_push(
    arena, (size_t) request.chart_count * sizeof(*visual->charts),
    sizeof(uint16_t));
  if (!visual->animations || !visual->charts ||
      !sf_caf_load_selected_animations(
        path, selections, request.chart_count,
        selected_parts, available_parts, arena, visual->animations))
    return false;
  memcpy(
    visual->charts, request.charts,
    (size_t) request.chart_count * sizeof(*visual->charts));
  visual->animation_count = request.chart_count;
  for (chart_index = 0u; chart_index < request.chart_count; ++chart_index) {
    const SfCafSelectedAnimation *animation =
      &visual->animations[chart_index];
    for (part = 0u; part < animation->part_count; ++part) {
      uint16_t frame;
      for (frame = 0u; frame < animation->frame_count; ++frame) {
        const SfCafCell *cell = &animation->parts[part].cells[frame];
        if ((cell->status & 8) != 0) {
          if (!sf_ground_item_add_value(
                shadow_patterns, &shadow_count,
                SF_GROUND_ITEM_PATTERN_LIMIT, cell->pattern)) return false;
        } else {
          if (!sf_ground_item_add_value(
                artwork_patterns, &artwork_count,
                SF_GROUND_ITEM_PATTERN_LIMIT, cell->pattern)) return false;
          if (animation->palette_mode == 1 &&
              !sf_ground_item_add_value(
                palettes, &palette_count,
                SF_NJP_SPARSE_PALETTE_LIMIT,
                animation->chart_priority_stride *
                  request.charts[chart_index] + cell->priority)) return false;
        }
      }
    }
  }
  if (artwork_count == 0u || shadow_count == 0u ||
      !sf_ground_item_path(
        path, sizeof(path), data_root,
        sf_retail_ground_item_paths.artwork_format,
        request.resource_id) ||
      !sf_njp_load_sparse_patterns_with_palette_capacity(
        path, artwork_patterns, artwork_count,
        palettes, (uint8_t) palette_count,
        SF_NJP_SPARSE_PALETTE_LIMIT, arena, &visual->artwork) ||
      !sf_ground_item_path(
        path, sizeof(path), data_root,
        sf_retail_ground_item_paths.shadow_format,
        request.resource_id) ||
      !sf_njp_load_sparse_patterns(
        path, shadow_patterns, shadow_count, arena, &visual->shadows))
    return false;
  return true;
}

bool sf_ground_item_assets_load(
    SfGroundItemAssets *assets, const char *data_root,
    const SfScsScript *script,
    const SfItemReference *retained_items, uint8_t retained_item_count,
    SfArena *arena) {
  SfGroundItemResourceRequest requests[SF_GROUND_ITEM_RESOURCE_LIMIT];
  uint8_t request_count;
  uint8_t index;
  char path[SF_RETAIL_PATH_CAPACITY];
  size_t mark;
  bool success = false;
  static const uint16_t samples[7] = {
    15u, 16u, 85u, 47u, 48u, 49u, 93u};
  if (!assets || !data_root || !script || !arena) return false;
  mark = sf_arena_mark(arena);
  memset(assets, 0, sizeof(*assets));
  memset(requests, 0, sizeof(requests));
  assets->definitions = (SfItemGroundDefinition *) sf_arena_push_zero(
    arena, SF_GROUND_ITEM_DEFINITION_LIMIT * sizeof(*assets->definitions),
    sizeof(void *));
  if (!assets->definitions || !sf_ground_item_collect_definitions(
        script, assets->definitions, &assets->definition_count)) goto done;
  for (index = 0u; index < retained_item_count; ++index) {
    uint8_t existing;
    if (!retained_items || retained_items[index].category > 4u) goto done;
    for (existing = 0u; existing < assets->definition_count; ++existing)
      if (assets->definitions[existing].category ==
            retained_items[index].category &&
          assets->definitions[existing].definition_id ==
            retained_items[index].definition_id) break;
    if (existing < assets->definition_count) continue;
    if (assets->definition_count >= SF_GROUND_ITEM_DEFINITION_LIMIT) goto done;
    assets->definitions[assets->definition_count].category =
      retained_items[index].category;
    assets->definitions[assets->definition_count].definition_id =
      retained_items[index].definition_id;
    ++assets->definition_count;
  }
  if (!sf_retail_path_join(
        path, sizeof(path), data_root,
        sf_retail_game_paths.item_database) ||
      !sf_item_read_ground_definitions(
        path, assets->definitions, assets->definition_count) ||
      !sf_ground_item_collect_resources(
        assets->definitions, assets->definition_count,
        requests, &request_count)) goto done;
  assets->visuals = (SfGroundItemVisual *) sf_arena_push_zero(
    arena, (size_t) request_count * sizeof(*assets->visuals), sizeof(void *));
  if (!assets->visuals) goto done;
  for (index = 0u; index < request_count; ++index)
    if (!sf_ground_item_visual_load(
          &assets->visuals[index], data_root, requests[index], arena))
      goto done;
  assets->visual_count = request_count;
  if (!sf_retail_path_join(
        path, sizeof(path), data_root,
        sf_retail_game_paths.common_sounds) ||
      !sf_voc_load_u8_mono_samples(
        path, samples, 7u, arena, assets->sounds)) goto done;
  assets->memory_bytes = sf_arena_mark(arena) - mark;
  success = true;
done:
  if (!success) {
    (void) sf_arena_rewind(arena, mark);
    memset(assets, 0, sizeof(*assets));
  }
  return success;
}

const SfPcmU8 *sf_ground_item_sound(
    const SfGroundItemAssets *assets, uint16_t sample) {
  static const uint16_t samples[7] = {
    15u, 16u, 85u, 47u, 48u, 49u, 93u};
  uint8_t index;
  if (!assets) return NULL;
  for (index = 0u; index < 7u; ++index)
    if (samples[index] == sample) return &assets->sounds[index];
  return NULL;
}

const SfGroundItemVisual *sf_ground_item_visual(
    const SfGroundItemAssets *assets, int32_t resource_id) {
  uint8_t index;
  if (!assets) return NULL;
  for (index = 0u; index < assets->visual_count; ++index)
    if (assets->visuals[index].resource_id == resource_id)
      return &assets->visuals[index];
  return NULL;
}

const SfCafSelectedAnimation *sf_ground_item_animation(
    const SfGroundItemVisual *visual, int32_t chart) {
  uint8_t index;
  if (!visual) return NULL;
  for (index = 0u; index < visual->animation_count; ++index)
    if (visual->charts[index] == chart) return &visual->animations[index];
  return NULL;
}
