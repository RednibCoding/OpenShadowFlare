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

#ifndef SHADOWFLARE_DATA_SAVE_GAME_H
#define SHADOWFLARE_DATA_SAVE_GAME_H

#include "data/save_player.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_SAVED_PROGRESS_VALUE_LIMIT 1000u

typedef struct SfSavedProgress {
  int32_t quest_values[SF_SAVED_PROGRESS_VALUE_LIMIT];
  int32_t transport_values[SF_SAVED_PROGRESS_VALUE_LIMIT];
  int32_t script_values[SF_SAVED_PROGRESS_VALUE_LIMIT];
  uint16_t quest_count;
  uint16_t transport_count;
  uint16_t script_count;
  bool present;
} SfSavedProgress;

typedef struct SfSavedWorldState {
  int32_t scenario_id;
  int32_t entry_value;
  int32_t mine_count;
  bool running;
  bool has_mine_count;
  bool present;
} SfSavedWorldState;

typedef struct SfSavedGame {
  SfSavedPlayer player;
  SfSavedProgress progress;
  SfSavedWorldState world;
} SfSavedGame;

bool sf_save_game_load(
  const char *data_root, uint8_t file_slot, SfSavedGame *game);
bool sf_save_game_load_path(const char *path, SfSavedGame *game);

#endif
