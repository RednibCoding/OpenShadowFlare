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

#include "data/save_game_internal.h"

#include <string.h>

#define SF_SAVED_MAGIC_VALUE_COUNT (22u * 3u + 8u)
#define SF_SAVED_COMPANION_LIMIT 256u

static bool sf_saved_game_i32(
    SfSavePayloadReader *reader, int32_t *value) {
  uint8_t bytes[4];
  if (sf_save_payload_content_remaining(reader) < sizeof(bytes) ||
      !sf_save_payload_read(reader, bytes, sizeof(bytes))) return false;
  *value = (int32_t) ((uint32_t) bytes[0] |
    ((uint32_t) bytes[1] << 8u) | ((uint32_t) bytes[2] << 16u) |
    ((uint32_t) bytes[3] << 24u));
  return true;
}

static bool sf_saved_game_array(
    SfSavePayloadReader *reader, int32_t *values, uint16_t *stored_count) {
  int32_t count;
  int32_t index;
  if (!sf_saved_game_i32(reader, &count) || count < 0 ||
      count > (int32_t) SF_SAVED_PROGRESS_VALUE_LIMIT ||
      sf_save_payload_content_remaining(reader) < (uint32_t) count * 4u)
    return false;
  *stored_count = (uint16_t) count;
  for (index = 0; index < count; ++index)
    if (!sf_saved_game_i32(reader, &values[index])) return false;
  return true;
}

static void sf_saved_game_swap_progress(SfSavedProgress *progress) {
  uint16_t index;
  const uint16_t count = progress->quest_count;
  progress->quest_count = progress->script_count;
  progress->script_count = count;
  for (index = 0u; index < SF_SAVED_PROGRESS_VALUE_LIMIT; ++index) {
    const int32_t value = progress->quest_values[index];
    progress->quest_values[index] = progress->script_values[index];
    progress->script_values[index] = value;
  }
}

static bool sf_saved_game_progress(
    SfSavePayloadReader *reader, SfSavedProgress *progress) {
  if (!sf_saved_game_array(
        reader, progress->quest_values, &progress->quest_count) ||
      !sf_saved_game_array(
        reader, progress->transport_values, &progress->transport_count) ||
      !sf_saved_game_array(
        reader, progress->script_values, &progress->script_count))
    return false;
  if (reader->extension_present && reader->extension_version == 1u)
    sf_saved_game_swap_progress(progress);
  progress->present = true;
  return true;
}

static bool sf_saved_game_skip_magic(SfSavePayloadReader *reader) {
  int32_t count;
  const uint32_t bytes = SF_SAVED_MAGIC_VALUE_COUNT * 4u;
  return sf_saved_game_i32(reader, &count) && count == 22 &&
    sf_save_payload_content_remaining(reader) >= bytes &&
    sf_save_payload_skip(reader, bytes);
}

static bool sf_saved_game_skip_companions(SfSavePayloadReader *reader) {
  int32_t count;
  uint32_t bytes;
  if (!sf_saved_game_i32(reader, &count) || count <= 0 ||
      count > (int32_t) SF_SAVED_COMPANION_LIMIT) return false;
  bytes = (uint32_t) count * 8u;
  return sf_save_payload_content_remaining(reader) >= bytes &&
    sf_save_payload_skip(reader, bytes);
}

static bool sf_saved_game_world(
    SfSavePayloadReader *reader, SfSavedWorldState *world) {
  int32_t running;
  if (!sf_saved_game_i32(reader, &world->mine_count)) return false;
  world->has_mine_count = true;
  if (sf_save_payload_content_remaining(reader) == 0u) return true;
  if (!sf_saved_game_i32(reader, &running) ||
      !sf_saved_game_i32(reader, &world->scenario_id) ||
      !sf_saved_game_i32(reader, &world->entry_value) ||
      world->scenario_id < 0 || world->entry_value < 0) return false;
  world->running = running != 0;
  world->present = true;
  return true;
}

bool sf_save_game_read_progress(
    SfSavePayloadReader *reader, SfSavedGame *game) {
  if (!reader || !game) return false;
  if (sf_save_payload_content_remaining(reader) == 0u) return true;
  if (!sf_saved_game_progress(reader, &game->progress)) return false;
  if (sf_save_payload_content_remaining(reader) == 0u) return true;
  if (!sf_saved_game_skip_magic(reader)) return false;
  if (sf_save_payload_content_remaining(reader) == 0u) return true;
  if (!sf_saved_game_skip_companions(reader)) return false;
  if (sf_save_payload_content_remaining(reader) == 0u) return true;
  return sf_saved_game_world(reader, &game->world);
}
