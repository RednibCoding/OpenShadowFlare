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

#include "assets/character_create_assets.h"

#include "assets/retail_paths.h"

#include <string.h>

bool sf_character_create_assets_load(
    SfCharacterCreateAssets *assets, const char *data_root, SfArena *arena) {
  static const uint8_t artwork_patterns[] = {
    0u, 1u, 2u, 3u, 4u, 5u,
    7u, 8u, 9u, 10u, 11u, 12u,
    20u, 21u, 22u, 23u, 24u, 25u,
    26u, 27u, 28u, 29u,
    34u, 35u, 36u, 38u, 41u
  };
  static const uint8_t font_pattern = 0u;
  char path[SF_RETAIL_PATH_CAPACITY];
  size_t mark;
  if (!assets || !data_root || !arena) return false;
  mark = sf_arena_mark(arena);
  memset(assets, 0, sizeof(*assets));
  if (!sf_retail_path_join(
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
        path, &font_pattern, 1u, arena, &assets->font)) {
    (void) sf_arena_rewind(arena, mark);
    memset(assets, 0, sizeof(*assets));
    return false;
  }
  assets->memory_bytes = sf_arena_mark(arena) - mark;
  assets->loaded = true;
  return true;
}
