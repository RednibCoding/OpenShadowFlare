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

#include "game/player_companion.h"

#include <string.h>

void sf_player_companion_progress_init(SfPlayerCompanionProgress *progress) {
  uint8_t index;
  if (!progress) return;
  memset(progress, 0, sizeof(*progress));
  for (index = 0u; index < SF_COMPANION_COUNT; ++index)
    progress->levels[index] = 1;
}

bool sf_player_companion_progress_restore(
    SfPlayerCompanionProgress *progress, int32_t type,
    int32_t defeated_updates, const int32_t *levels,
    const int32_t *experience, uint8_t count) {
  uint8_t index;
  if (!progress || !levels || !experience || count != SF_COMPANION_COUNT ||
      type < 0 || type >= (int32_t) SF_COMPANION_COUNT ||
      defeated_updates < 0) return false;
  for (index = 0u; index < count; ++index) {
    if (levels[index] < 1 || levels[index] > 35 || experience[index] < 0)
      return false;
  }
  progress->type = type;
  progress->defeated_updates = defeated_updates;
  memcpy(progress->levels, levels, sizeof(progress->levels));
  memcpy(progress->experience, experience, sizeof(progress->experience));
  return true;
}

int32_t sf_player_companion_level(const SfPlayerCompanionProgress *progress) {
  if (!progress || progress->type < 0 ||
      progress->type >= (int32_t) SF_COMPANION_COUNT) return 0;
  return progress->levels[progress->type];
}
