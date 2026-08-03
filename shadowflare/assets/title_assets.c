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

#include "assets/title_assets.h"

#include "assets/retail_paths.h"

#include <stdio.h>
#include <string.h>

bool sf_title_assets_load(
    SfTitleAssets *assets, const char *data_root, SfArena *arena,
    void *decode_scratch, size_t decode_scratch_size) {
  static const uint8_t title_patterns[4] = {0u, 1u, 2u, 3u};
  char path[SF_RETAIL_PATH_CAPACITY];
  size_t mark;
  unsigned smoke;
  if (!assets || !data_root || !data_root[0] || !arena || !decode_scratch ||
      decode_scratch_size < SF_TITLE_DECODE_SCRATCH_BYTES) return false;
  mark = sf_arena_mark(arena);
  memset(assets, 0, sizeof(*assets));
  if (!sf_retail_path_join(path, sizeof(path), data_root,
        sf_retail_title_paths.artwork) ||
      !sf_njp_load_selected(
        path, title_patterns, 4u, arena, &assets->artwork)) goto failed;
  for (smoke = 0u; smoke < SF_TITLE_SMOKE_COUNT; ++smoke) {
    SfTitleSmokeAsset *asset = &assets->smoke[smoke];
    uint8_t frame;
    char relative[128];
    int length = snprintf(relative, sizeof(relative),
      sf_retail_title_paths.smoke_artwork_format, smoke);
    if (length <= 0 || (size_t) length >= sizeof(relative) ||
        !sf_retail_path_join(path, sizeof(path), data_root, relative) ||
        !sf_njp_load_animation(path, arena, &asset->images) ||
        !sf_njp_find_blank_frames(
          &asset->images, decode_scratch, decode_scratch_size)) goto failed;
    length = snprintf(relative, sizeof(relative),
      sf_retail_title_paths.smoke_animation_format, smoke);
    if (length <= 0 || (size_t) length >= sizeof(relative) ||
        !sf_retail_path_join(path, sizeof(path), data_root, relative) ||
        !sf_caf_load_first_chart_direction(path, 8u, &asset->animation) ||
        asset->animation.frame_count != asset->images.frame_count) goto failed;
    for (frame = 0u; frame < asset->animation.frame_count; ++frame) {
      const int pattern = asset->animation.frames[frame].pattern;
      if (pattern < 0 || pattern >= asset->images.frame_count) goto failed;
      if (asset->images.frames[pattern].decoded_size >
          assets->decode_scratch_bytes) {
        assets->decode_scratch_bytes =
          asset->images.frames[pattern].decoded_size;
      }
    }
  }
  assets->memory_bytes = sf_arena_mark(arena) - mark;
  assets->loaded = true;
  return true;
failed:
  (void) sf_arena_rewind(arena, mark);
  memset(assets, 0, sizeof(*assets));
  return false;
}
