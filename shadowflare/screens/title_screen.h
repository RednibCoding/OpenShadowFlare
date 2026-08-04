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

#ifndef SHADOWFLARE_SCREENS_TITLE_SCREEN_H
#define SHADOWFLARE_SCREENS_TITLE_SCREEN_H

#include "assets/title_assets.h"
#include "game/game.h"
#include "render/dirty.h"
#include "render/renderer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct SfTitleScreen {
  SfDirtyRects dirty;
  int16_t previous_smoke_frame[SF_GAME_TITLE_SMOKE_COUNT];
  uint16_t previous_menu_brightness[SF_GAME_TITLE_ENTRY_COUNT];
  uint16_t previous_scene_brightness;
  uint8_t previous_menu_visible;
  void *decode_scratch;
  size_t decode_scratch_size;
  bool initialized;
} SfTitleScreen;

bool sf_title_screen_init(
  SfTitleScreen *title, void *scratch, size_t scratch_size,
  size_t required_scratch_size);
void sf_title_screen_draw(
  SfTitleScreen *title, SfRenderer *renderer,
  const SfTitleAssets *assets, const SfGame *game);

#endif
