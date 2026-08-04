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

#ifndef SHADOWFLARE_ASSETS_GAMEPLAY_SOUND_ASSETS_H
#define SHADOWFLARE_ASSETS_GAMEPLAY_SOUND_ASSETS_H

#include "core/arena.h"
#include "data/voc.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_GAMEPLAY_SOUND_COUNT 3u

typedef struct SfGameplaySoundAssets {
  SfPcmU8 sounds[SF_GAMEPLAY_SOUND_COUNT];
} SfGameplaySoundAssets;

bool sf_gameplay_sound_assets_load(
  SfGameplaySoundAssets *assets, const char *data_root, SfArena *arena);
const SfPcmU8 *sf_gameplay_sound(
  const SfGameplaySoundAssets *assets, uint16_t sample);

#endif
