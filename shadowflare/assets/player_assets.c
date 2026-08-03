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

#include "assets/player_assets.h"

#include "assets/retail_paths.h"

#include <stdio.h>
#include <string.h>

static bool sf_player_path(
    char *path, size_t capacity, const char *root,
    const char *format, const char *gender) {
  char relative[SF_RETAIL_PATH_CAPACITY];
  const int length = snprintf(relative, sizeof(relative), format, gender);
  return length >= 0 && (size_t) length < sizeof(relative) &&
    sf_retail_path_join(path, capacity, root, relative);
}

static bool sf_player_add_pattern(
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

bool sf_player_assets_load(
    SfPlayerAssets *assets, const char *data_root,
    uint8_t gender, uint8_t direction,
    const uint8_t *appearance_parts, uint8_t appearance_part_count,
    const SfItemReference *visible_items, uint8_t visible_item_count,
    SfArena *arena) {
  uint8_t selected_parts[SF_CAF_SELECTED_PART_LIMIT];
  int32_t artwork_patterns[SF_NJP_SPARSE_PATTERN_LIMIT];
  int32_t shadow_patterns[SF_NJP_SPARSE_PATTERN_LIMIT];
  uint16_t artwork_count = 0u;
  uint16_t shadow_count = 0u;
  const char *gender_name = gender == 1u ? "Male" : "Female";
  char path[SF_RETAIL_PATH_CAPACITY];
  size_t mark;
  uint8_t part;
  bool success = false;
  if (!assets || !data_root || direction > 7u || !appearance_parts ||
      appearance_part_count == 0u ||
      appearance_part_count + visible_item_count > SF_CAF_SELECTED_PART_LIMIT ||
      (visible_item_count > 0u && !visible_items) || !arena) return false;
  mark = sf_arena_mark(arena);
  memset(assets, 0, sizeof(*assets));
  memcpy(selected_parts, appearance_parts, appearance_part_count);
  if (visible_item_count > 0u) {
    char item_path[SF_RETAIL_PATH_CAPACITY];
    uint8_t item;
    if (!sf_retail_path_join(
          item_path, sizeof(item_path), data_root,
          sf_retail_game_paths.item_database)) goto done;
    for (item = 0u; item < visible_item_count; ++item) {
      SfItemAppearance appearance;
      if (!sf_item_read_appearance(
            item_path, visible_items[item].category,
            visible_items[item].definition_id, &appearance) ||
          appearance.part < 0 || appearance.part > UINT8_MAX) goto done;
      selected_parts[appearance_part_count++] = (uint8_t) appearance.part;
    }
  }
  if (!sf_player_path(
        path, sizeof(path), data_root,
        sf_retail_player_paths.animation_format, gender_name) ||
      !sf_caf_load_selected_chart_direction(
        path, 0u, direction, selected_parts, appearance_part_count,
        arena, &assets->idle)) goto done;
  for (part = 0u; part < assets->idle.part_count; ++part) {
    uint8_t frame;
    for (frame = 0u; frame < assets->idle.frame_count; ++frame) {
      const SfCafCell *cell = &assets->idle.parts[part].cells[frame];
      if (((cell->status & 8) == 0 && !sf_player_add_pattern(
            artwork_patterns, &artwork_count, cell->pattern)) ||
          ((cell->status & 8) != 0 && !sf_player_add_pattern(
            shadow_patterns, &shadow_count, cell->pattern))) goto done;
    }
  }
  if (artwork_count == 0u || shadow_count == 0u ||
      !sf_player_path(
        path, sizeof(path), data_root,
        sf_retail_player_paths.artwork_format, gender_name) ||
      !sf_njp_load_sparse_patterns(
        path, artwork_patterns, artwork_count, arena, &assets->artwork) ||
      !sf_player_path(
        path, sizeof(path), data_root,
        sf_retail_player_paths.shadow_format, gender_name) ||
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
