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

#ifndef SHADOWFLARE_GAME_GAME_H
#define SHADOWFLARE_GAME_GAME_H

#include <stdbool.h>
#include <stdint.h>

typedef enum SfGameMode {
  SF_GAME_MODE_TITLE = 0
} SfGameMode;

typedef struct SfGameInput {
  bool up_pressed;
  bool down_pressed;
  bool confirm_pressed;
  bool cancel_pressed;
} SfGameInput;

typedef struct SfGame {
  SfGameMode mode;
  uint32_t ticks;
  uint8_t title_selection;
  bool quit_requested;
} SfGame;

void sf_game_init(SfGame *game);
void sf_game_update(SfGame *game, const SfGameInput *input);

#endif
