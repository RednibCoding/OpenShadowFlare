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

#ifndef SHADOWFLARE_ASSETS_LOAD_GAME_ASSETS_H
#define SHADOWFLARE_ASSETS_LOAD_GAME_ASSETS_H

#include "core/arena.h"
#include "data/njp.h"
#include "data/save.h"
#include "render/renderer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SF_SAVE_PREVIEW_WIDTH 391u
#define SF_SAVE_PREVIEW_HEIGHT 114u
#define SF_SAVE_PREVIEW_PIXELS \
  (SF_SAVE_PREVIEW_WIDTH * SF_SAVE_PREVIEW_HEIGHT)

typedef struct SfLoadGameAssets {
  SfNjpDecodedResource artwork;
  SfNjpSelected font;
  SfSaveCatalog catalog;
  SfRgb555Image preview;
  uint16_t *preview_pixels;
  size_t memory_bytes;
  int8_t preview_catalog_index;
  bool loaded;
} SfLoadGameAssets;

bool sf_load_game_assets_load(
  SfLoadGameAssets *assets, const char *data_root, SfArena *arena,
  void *scratch, size_t scratch_size);
bool sf_load_game_assets_select_preview(
  SfLoadGameAssets *assets, const char *data_root, uint8_t catalog_index,
  void *scratch, size_t scratch_size);
bool sf_load_game_assets_delete(
  SfLoadGameAssets *assets, const char *data_root, uint8_t catalog_index,
  void *scratch, size_t scratch_size);

#endif
