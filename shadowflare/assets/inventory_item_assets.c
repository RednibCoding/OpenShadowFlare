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

#include "assets/inventory_item_assets.h"

#include "assets/retail_paths.h"

#include <stdio.h>
#include <string.h>

#define SF_INVENTORY_ARTWORK_PATTERN_LIMIT 16u

static bool sf_inventory_add_value(
    int32_t *values, uint8_t *count, uint8_t capacity, int32_t value) {
  uint8_t index;
  if (value < 0) return false;
  for (index = 0u; index < *count; ++index)
    if (values[index] == value) return true;
  if (*count >= capacity) return false;
  values[(*count)++] = value;
  return true;
}

static bool sf_inventory_add_optional_value(
    int32_t *values, uint8_t *count, uint8_t capacity, int32_t value) {
  return value < 0 || sf_inventory_add_value(
    values, count, capacity, value);
}

bool sf_inventory_item_assets_load(
    SfInventoryItemAssets *assets, const char *data_root,
    const SfItemGroundDefinition *definitions, uint8_t definition_count,
    SfArena *arena) {
  size_t mark;
  uint8_t group;
  if (!assets || !data_root || !definitions || definition_count == 0u ||
      !arena) return false;
  mark = sf_arena_mark(arena);
  memset(assets, 0, sizeof(*assets));
  for (group = 0u; group < SF_INVENTORY_ARTWORK_GROUP_LIMIT; ++group) {
    int32_t patterns[SF_INVENTORY_ARTWORK_PATTERN_LIMIT];
    int32_t palettes[SF_NJP_SPARSE_PALETTE_LIMIT];
    uint8_t pattern_count = 0u;
    uint8_t palette_count = 0u;
    uint8_t definition;
    char relative[64];
    char path[SF_RETAIL_PATH_CAPACITY];
    int length;
    SfInventoryArtworkGroup *output;
    for (definition = 0u; definition < definition_count; ++definition) {
      const SfItemGroundDefinition *item = &definitions[definition];
      if (item->inventory_pattern_group != group) continue;
      if (!sf_inventory_add_value(
            patterns, &pattern_count, SF_INVENTORY_ARTWORK_PATTERN_LIMIT,
            item->inventory_pattern) ||
          !sf_inventory_add_optional_value(
            palettes, &palette_count, SF_NJP_SPARSE_PALETTE_LIMIT,
            item->inventory_palette)) goto failed;
    }
    if (pattern_count == 0u) continue;
    if (assets->group_count >= SF_INVENTORY_ARTWORK_GROUP_LIMIT) goto failed;
    length = snprintf(
      relative, sizeof(relative),
      sf_retail_game_paths.inventory_item_format, (unsigned) group);
    if (length < 0 || (size_t) length >= sizeof(relative) ||
        !sf_retail_path_join(
          path, sizeof(path), data_root, relative)) goto failed;
    output = &assets->groups[assets->group_count];
    if (!sf_njp_load_sparse_patterns_with_palette_capacity(
          path, patterns, pattern_count, palettes, palette_count,
          SF_NJP_SPARSE_PALETTE_LIMIT,
          arena, &output->resource)) goto failed;
    output->source_group = group;
    ++assets->group_count;
  }
  return assets->group_count > 0u;
failed:
  (void) sf_arena_rewind(arena, mark);
  memset(assets, 0, sizeof(*assets));
  return false;
}

const SfNjpSparseResource *sf_inventory_item_artwork(
    const SfInventoryItemAssets *assets, int32_t source_group) {
  uint8_t index;
  if (!assets || source_group < 0 || source_group > UINT8_MAX) return NULL;
  for (index = 0u; index < assets->group_count; ++index)
    if (assets->groups[index].source_group == (uint8_t) source_group)
      return &assets->groups[index].resource;
  return NULL;
}
