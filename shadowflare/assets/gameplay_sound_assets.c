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

#include "assets/gameplay_sound_assets.h"

#include "assets/retail_paths.h"

#include <string.h>

static const uint16_t sf_gameplay_samples[
    SF_GAMEPLAY_SOUND_COUNT] = {57u, 58u, 80u};

bool sf_gameplay_sound_assets_load(
    SfGameplaySoundAssets *assets, const char *data_root, SfArena *arena) {
  char path[SF_RETAIL_PATH_CAPACITY];
  if (!assets || !data_root || !arena) return false;
  memset(assets, 0, sizeof(*assets));
  return sf_retail_path_join(
      path, sizeof(path), data_root, sf_retail_game_paths.common_sounds) &&
    sf_voc_load_u8_mono_samples(
      path, sf_gameplay_samples,
      SF_GAMEPLAY_SOUND_COUNT, arena, assets->sounds);
}

const SfPcmU8 *sf_gameplay_sound(
    const SfGameplaySoundAssets *assets, uint16_t sample) {
  uint8_t index;
  if (!assets) return NULL;
  for (index = 0u; index < SF_GAMEPLAY_SOUND_COUNT; ++index)
    if (sf_gameplay_samples[index] == sample)
      return &assets->sounds[index];
  return NULL;
}
