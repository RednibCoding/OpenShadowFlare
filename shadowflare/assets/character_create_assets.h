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

#ifndef SHADOWFLARE_ASSETS_CHARACTER_CREATE_ASSETS_H
#define SHADOWFLARE_ASSETS_CHARACTER_CREATE_ASSETS_H

#include "core/arena.h"
#include "data/njp.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct SfCharacterCreateAssets {
  SfNjpDecodedResource artwork;
  SfNjpSelected font;
  size_t memory_bytes;
  bool loaded;
} SfCharacterCreateAssets;

bool sf_character_create_assets_load(
  SfCharacterCreateAssets *assets, const char *data_root, SfArena *arena);

#endif
