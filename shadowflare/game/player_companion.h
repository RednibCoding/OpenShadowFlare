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

#ifndef SHADOWFLARE_GAME_PLAYER_COMPANION_H
#define SHADOWFLARE_GAME_PLAYER_COMPANION_H

#include "data/companion_parameters.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct SfPlayerCompanionProgress {
  int32_t levels[SF_COMPANION_COUNT];
  int32_t experience[SF_COMPANION_COUNT];
  int32_t type;
  int32_t defeated_updates;
} SfPlayerCompanionProgress;

void sf_player_companion_progress_init(SfPlayerCompanionProgress *progress);
bool sf_player_companion_progress_restore(
  SfPlayerCompanionProgress *progress, int32_t type,
  int32_t defeated_updates, const int32_t *levels,
  const int32_t *experience, uint8_t count);
int32_t sf_player_companion_level(const SfPlayerCompanionProgress *progress);

#endif
