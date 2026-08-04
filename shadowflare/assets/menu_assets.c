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

#include "assets/menu_assets.h"

#include "assets/retail_paths.h"

#include <string.h>

bool sf_menu_assets_load(
    SfMenuAssets *assets, const char *data_root, SfArena *arena) {
  static const uint16_t sound_indices[SF_MENU_SOUND_COUNT] = {
    55u, 56u, 58u, 62u};
  static const uint16_t music_index = 0u;
  char path[SF_RETAIL_PATH_CAPACITY];
  size_t mark;
  if (!assets || !data_root || !arena) return false;
  mark = sf_arena_mark(arena);
  memset(assets, 0, sizeof(*assets));
  if (!sf_retail_path_join(
        path, sizeof(path), data_root, sf_retail_menu_paths.music) ||
      !sf_voc_load_u8_mono_samples(
        path, &music_index, 1u, arena, &assets->music) ||
      !sf_retail_path_join(
        path, sizeof(path), data_root, sf_retail_menu_paths.common_sounds) ||
      !sf_voc_load_u8_mono_samples(
        path, sound_indices, SF_MENU_SOUND_COUNT, arena, assets->sounds)) {
    (void) sf_arena_rewind(arena, mark);
    memset(assets, 0, sizeof(*assets));
    return false;
  }
  assets->loaded = true;
  return true;
}
