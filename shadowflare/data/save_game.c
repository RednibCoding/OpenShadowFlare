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

#include "data/save_game.h"

#include "assets/retail_paths.h"
#include "data/save.h"
#include "data/save_game_internal.h"
#include "data/save_payload.h"
#include "data/save_player_internal.h"

#include <string.h>

static void sf_save_game_extension(
    SfSavedGame *game, const SfSavePayloadReader *reader) {
  game->world.mine_count = 5;
  if (!reader->extension_present) return;
  game->world.running = reader->extension_running;
  if (reader->extension_has_mine_count) {
    game->world.mine_count = reader->extension_mine_count;
    game->world.has_mine_count = true;
  }
}

bool sf_save_game_load_path(const char *path, SfSavedGame *game) {
  SfSavePayloadReader reader;
  uint8_t record[SF_SAVE_PLAYER_RECORD_SIZE];
  uint8_t duplicate[SF_SAVE_PLAYER_RECORD_SIZE];
  bool has_envelope;
  bool success = false;
  if (!path || !game) return false;
  memset(game, 0, sizeof(*game));
  game->world.mine_count = 5;
  if (!sf_save_payload_open(&reader, path, record, &has_envelope) ||
      !sf_save_player_read_record(record, &game->player)) goto done;
  if (!has_envelope) {
    success = true;
    goto done;
  }
  sf_save_game_extension(game, &reader);
  if (!sf_save_payload_read(&reader, duplicate, sizeof(duplicate)) ||
      memcmp(record, duplicate, sizeof(record)) != 0 ||
      !sf_save_player_read_items(&reader, &game->player) ||
      !sf_save_game_read_progress(&reader, game) ||
      !sf_save_payload_finish(&reader)) goto done;
  success = true;
done:
  sf_save_payload_close(&reader);
  if (!success) memset(game, 0, sizeof(*game));
  return success;
}

bool sf_save_game_load(
    const char *data_root, uint8_t file_slot, SfSavedGame *game) {
  char path[SF_RETAIL_PATH_CAPACITY];
  return data_root && game &&
    sf_save_slot_data_path(path, sizeof(path), data_root, file_slot) &&
    sf_save_game_load_path(path, game);
}
