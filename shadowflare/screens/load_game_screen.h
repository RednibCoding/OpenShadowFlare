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

#ifndef SHADOWFLARE_SCREENS_LOAD_GAME_SCREEN_H
#define SHADOWFLARE_SCREENS_LOAD_GAME_SCREEN_H

#include "assets/load_game_assets.h"
#include "game/game.h"
#include "render/renderer.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct SfLoadGameScreen {
  SfLoadGameState previous;
  int8_t previous_preview_index;
  uint8_t previous_save_count;
  bool initialized;
} SfLoadGameScreen;

void sf_load_game_screen_init(SfLoadGameScreen *screen);
void sf_load_game_screen_draw(
  SfLoadGameScreen *screen, SfRenderer *renderer,
  const SfLoadGameAssets *assets, const SfGame *game);

#endif
