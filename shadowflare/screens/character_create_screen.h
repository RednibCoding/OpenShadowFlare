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

#ifndef SHADOWFLARE_SCREENS_CHARACTER_CREATE_SCREEN_H
#define SHADOWFLARE_SCREENS_CHARACTER_CREATE_SCREEN_H

#include "assets/character_create_assets.h"
#include "game/game.h"
#include "render/renderer.h"

#include <stdbool.h>

typedef struct SfCharacterCreateScreen {
  SfCharacterCreateState previous;
  bool initialized;
} SfCharacterCreateScreen;

void sf_character_create_screen_init(SfCharacterCreateScreen *screen);
void sf_character_create_screen_draw(
  SfCharacterCreateScreen *screen, SfRenderer *renderer,
  const SfCharacterCreateAssets *assets, const SfGame *game);

#endif
