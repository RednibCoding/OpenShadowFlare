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

#include "assets/load_game_assets.h"

#include "assets/retail_paths.h"
#include "data/bmp.h"

#include <stdio.h>
#include <string.h>

static bool sf_load_game_preview_path(
    char *path, size_t capacity, const char *data_root, uint8_t file_slot) {
  char relative[32];
  const int length = snprintf(
    relative, sizeof(relative),
    sf_retail_save_paths.preview_format, file_slot);
  return length >= 0 && (size_t) length < sizeof(relative) &&
    sf_retail_path_join(path, capacity, data_root, relative);
}

bool sf_load_game_assets_select_preview(
    SfLoadGameAssets *assets, const char *data_root, uint8_t catalog_index,
    void *scratch, size_t scratch_size) {
  char path[SF_RETAIL_PATH_CAPACITY];
  if (!assets || !assets->loaded || !data_root || !assets->preview_pixels ||
      catalog_index >= assets->catalog.count) return false;
  if (assets->preview_catalog_index == (int8_t) catalog_index) return true;
  memset(&assets->preview, 0, sizeof(assets->preview));
  assets->preview_catalog_index = (int8_t) catalog_index;
  if (!sf_load_game_preview_path(
        path, sizeof(path), data_root,
        assets->catalog.entries[catalog_index].file_slot)) return true;
  (void) sf_bmp_load_rgb555(
    path, assets->preview_pixels, SF_SAVE_PREVIEW_PIXELS,
    scratch, scratch_size, &assets->preview);
  return true;
}

bool sf_load_game_assets_load(
    SfLoadGameAssets *assets, const char *data_root, SfArena *arena,
    void *scratch, size_t scratch_size) {
  static const uint8_t artwork_patterns[] = {
    0u, 1u, 2u, 3u, 4u, 5u, 6u,
    14u, 15u, 16u, 17u,
    20u, 21u, 22u, 23u, 24u, 25u,
    26u, 27u, 28u, 29u,
    37u, 38u, 39u, 40u, 41u,
    42u, 43u, 44u, 45u, 46u, 47u, 48u, 49u,
    50u, 51u, 52u, 53u, 54u, 55u, 56u, 57u,
    58u, 59u, 60u, 61u, 62u, 63u, 64u, 65u
  };
  static const uint8_t font_pattern = 0u;
  char path[SF_RETAIL_PATH_CAPACITY];
  size_t mark;
  if (!assets || !data_root || !arena || !scratch || scratch_size == 0u)
    return false;
  mark = sf_arena_mark(arena);
  memset(assets, 0, sizeof(*assets));
  assets->preview_catalog_index = -1;
  if (!sf_save_catalog_load(data_root, &assets->catalog) ||
      !sf_retail_path_join(
        path, sizeof(path), data_root,
        sf_retail_character_create_paths.artwork) ||
      !sf_njp_load_decoded_patterns(
        path, artwork_patterns,
        (uint8_t) (sizeof(artwork_patterns) / sizeof(artwork_patterns[0])),
        arena, &assets->artwork) ||
      !sf_retail_path_join(
        path, sizeof(path), data_root,
        sf_retail_character_create_paths.font) ||
      !sf_njp_load_selected(
        path, &font_pattern, 1u, arena, &assets->font)) goto failed;
  if (assets->catalog.count > 0u) {
    assets->preview_pixels = (uint16_t *) sf_arena_push(
      arena, SF_SAVE_PREVIEW_PIXELS * sizeof(uint16_t), sizeof(uint16_t));
    if (!assets->preview_pixels) goto failed;
  }
  assets->memory_bytes = sf_arena_mark(arena) - mark;
  assets->loaded = true;
  if (assets->catalog.count > 0u)
    (void) sf_load_game_assets_select_preview(
      assets, data_root, 0u, scratch, scratch_size);
  return true;
failed:
  (void) sf_arena_rewind(arena, mark);
  memset(assets, 0, sizeof(*assets));
  return false;
}

bool sf_load_game_assets_delete(
    SfLoadGameAssets *assets, const char *data_root, uint8_t catalog_index,
    void *scratch, size_t scratch_size) {
  if (!assets || !assets->loaded ||
      !sf_save_catalog_delete(data_root, &assets->catalog, catalog_index) ||
      !sf_save_catalog_load(data_root, &assets->catalog)) return false;
  assets->preview_catalog_index = -1;
  memset(&assets->preview, 0, sizeof(assets->preview));
  if (assets->catalog.count > 0u)
    (void) sf_load_game_assets_select_preview(
      assets, data_root, 0u, scratch, scratch_size);
  return true;
}
