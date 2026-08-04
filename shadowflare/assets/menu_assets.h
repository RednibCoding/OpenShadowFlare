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

#ifndef SHADOWFLARE_ASSETS_MENU_ASSETS_H
#define SHADOWFLARE_ASSETS_MENU_ASSETS_H

#include "core/arena.h"
#include "data/voc.h"

#include <stdbool.h>

#define SF_MENU_SOUND_COUNT 4u

typedef enum SfMenuSound {
  SF_MENU_SOUND_CONFIRM = 0,
  SF_MENU_SOUND_TITLE_CONFIRM,
  SF_MENU_SOUND_MOVE,
  SF_MENU_SOUND_TITLE_CUE
} SfMenuSound;

typedef struct SfMenuAssets {
  SfPcmU8 music;
  SfPcmU8 sounds[SF_MENU_SOUND_COUNT];
  bool loaded;
} SfMenuAssets;

bool sf_menu_assets_load(
  SfMenuAssets *assets, const char *data_root, SfArena *arena);

#endif
