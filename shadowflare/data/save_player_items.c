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

#include "data/save_player_internal.h"

#include <string.h>

#define SF_SAVE_SPECIAL_ITEM_LIMIT 4096

static bool sf_saved_read_i32(
    SfSavePayloadReader *reader, int32_t *value) {
  uint8_t bytes[4];
  if (!sf_save_payload_read(reader, bytes, sizeof(bytes))) return false;
  *value = (int32_t) ((uint32_t) bytes[0] |
    ((uint32_t) bytes[1] << 8u) | ((uint32_t) bytes[2] << 16u) |
    ((uint32_t) bytes[3] << 24u));
  return true;
}

static int32_t sf_saved_state_size(int32_t category) {
  static const int32_t sizes[5] = {200, 200, 192, 0, 4};
  return category >= 0 && category < 5 ? sizes[category] : -1;
}

static bool sf_saved_read_item(
    SfSavePayloadReader *reader, bool has_grid, SfSavedItem *item) {
  uint8_t state[200];
  int32_t category;
  int32_t identified;
  int32_t state_size;
  int32_t expected_size;
  memset(item, 0, sizeof(*item));
  item->quantity = 1;
  if (!sf_saved_read_i32(reader, &category) ||
      !sf_saved_read_i32(reader, &item->definition_id) ||
      !sf_saved_read_i32(reader, &identified) ||
      (has_grid && (!sf_saved_read_i32(reader, &item->grid_x) ||
                    !sf_saved_read_i32(reader, &item->grid_y))) ||
      !sf_saved_read_i32(reader, &state_size)) return false;
  expected_size = sf_saved_state_size(category);
  if (expected_size < 0 || state_size != expected_size ||
      !sf_save_payload_read(reader, state, (size_t) state_size)) return false;
  item->category = (uint8_t) category;
  item->identified = identified != 0;
  item->present = true;
  if (category == 0 || category == 1) {
    item->durability = (int32_t) ((uint32_t) state[188] |
      ((uint32_t) state[189] << 8u) | ((uint32_t) state[190] << 16u) |
      ((uint32_t) state[191] << 24u));
  } else if (category == 4) {
    item->quantity = (int32_t) ((uint32_t) state[0] |
      ((uint32_t) state[1] << 8u) | ((uint32_t) state[2] << 16u) |
      ((uint32_t) state[3] << 24u));
    if (item->quantity <= 0 ||
        (item->definition_id == 0 && item->quantity > 10000) ||
        (item->definition_id != 0 && item->quantity != 1)) return false;
  }
  return true;
}

static bool sf_saved_read_equipment(
    SfSavePayloadReader *reader, SfSavedPlayer *player) {
  uint8_t index;
  for (index = 0u; index < SF_SAVED_EQUIPMENT_COUNT; ++index) {
    int32_t present;
    if (!sf_saved_read_i32(reader, &present) ||
        (present != 0 && present != 1)) return false;
    if (present == 1 && !sf_saved_read_item(
          reader, false, &player->equipment[index])) return false;
  }
  return true;
}

static bool sf_saved_read_container(
    SfSavePayloadReader *reader, SfSavedItem *items,
    uint8_t capacity, uint8_t *stored_count) {
  int32_t count;
  int32_t index;
  if (!sf_saved_read_i32(reader, &count) || count < 0 || count > capacity)
    return false;
  *stored_count = (uint8_t) count;
  for (index = 0; index < count; ++index)
    if (!sf_saved_read_item(reader, true, &items[index])) return false;
  return true;
}

static bool sf_saved_skip_special_items(SfSavePayloadReader *reader) {
  int32_t count;
  int32_t index;
  if (!sf_saved_read_i32(reader, &count) ||
      count < 0 || count > SF_SAVE_SPECIAL_ITEM_LIMIT) return false;
  for (index = 0; index < count; ++index) {
    SfSavedItem ignored;
    if (!sf_saved_read_item(reader, true, &ignored)) return false;
  }
  return true;
}

bool sf_save_player_read_items(
    SfSavePayloadReader *reader, SfSavedPlayer *player) {
  if (!reader || !player) return false;
  if (sf_save_payload_content_remaining(reader) == 0u) return true;
  return sf_saved_read_equipment(reader, player) &&
    sf_saved_read_container(
      reader, player->backpack, SF_SAVED_BACKPACK_ITEM_LIMIT,
      &player->backpack_count) &&
    sf_saved_read_container(
      reader, player->belt, SF_SAVED_BELT_ITEM_LIMIT,
      &player->belt_count) &&
    sf_saved_skip_special_items(reader);
}
