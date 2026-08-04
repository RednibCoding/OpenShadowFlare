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

#include "data/save_player.h"

#include "data/save_player_internal.h"

#include <string.h>

static int32_t sf_saved_i32(const uint8_t *bytes) {
  return (int32_t) ((uint32_t) bytes[0] |
    ((uint32_t) bytes[1] << 8u) | ((uint32_t) bytes[2] << 16u) |
    ((uint32_t) bytes[3] << 24u));
}

bool sf_save_player_read_record(
    const uint8_t record[SF_SAVED_PLAYER_RECORD_SIZE],
    SfSavedPlayer *player) {
  static const uint16_t parameter_offsets[SF_PLAYER_INITIAL_PARAMETER_COUNT] = {
    0x28u, 0x2cu, 0x30u, 0x38u, 0x40u, 0x44u, 0x48u,
    0x54u, 0x58u, 0x4cu, 0x50u, 0x5cu, 0x60u
  };
  uint8_t index;
  memcpy(player->name, record, 24u);
  player->name[24] = '\0';
  player->gender = sf_saved_i32(record + 0x18u);
  player->job = sf_saved_i32(record + 0x1cu);
  player->level = sf_saved_i32(record + 0x24u);
  player->current_life = sf_saved_i32(record + 0x34u);
  player->current_mana = sf_saved_i32(record + 0x3cu);
  player->experience = sf_saved_i32(record + 0xd8u);
  player->element_x = sf_saved_i32(record + 0x64u);
  player->element_y = sf_saved_i32(record + 0x68u);
  player->companion_type = sf_saved_i32(record + 0x140u);
  player->companion_level = sf_saved_i32(record + 0x144u);
  player->companion_experience = sf_saved_i32(record + 0x148u);
  player->companion_defeated_updates = sf_saved_i32(record + 0x14cu);
  for (index = 0u; index < SF_PLAYER_INITIAL_PARAMETER_COUNT; ++index)
    player->parameters.values[index] = sf_saved_i32(
      record + parameter_offsets[index]);
  return (player->gender == 0 || player->gender == 1) &&
    player->level > 0 && player->level <= 100 &&
    player->element_x >= -20000 && player->element_x <= 20000 &&
    player->element_y >= -20000 && player->element_y <= 20000 &&
    player->companion_type >= 0 &&
    player->companion_type < (int32_t) SF_COMPANION_COUNT &&
    player->companion_level >= 1 && player->companion_level <= 35 &&
    player->companion_experience >= 0 &&
    player->companion_defeated_updates >= 0 &&
    player->parameters.values[2] > 0 && player->parameters.values[3] > 0;
}

static bool sf_saved_player_add_item(
    SfItemReference *items, uint8_t *count, uint8_t capacity,
    const SfSavedItem *item) {
  uint8_t index;
  if (!item->present) return true;
  for (index = 0u; index < *count; ++index)
    if (items[index].category == item->category &&
        items[index].definition_id == item->definition_id) return true;
  if (*count >= capacity) return false;
  items[*count].category = item->category;
  items[*count].definition_id = item->definition_id;
  ++*count;
  return true;
}

bool sf_saved_player_required_items(
    const SfSavedPlayer *player, SfItemReference *items,
    uint8_t capacity, uint8_t *item_count) {
  uint8_t index;
  if (!player || !items || !item_count || capacity == 0u) return false;
  *item_count = 0u;
  for (index = 0u; index < SF_SAVED_EQUIPMENT_COUNT; ++index)
    if (!sf_saved_player_add_item(
          items, item_count, capacity, &player->equipment[index]))
      return false;
  for (index = 0u; index < player->backpack_count; ++index)
    if (!sf_saved_player_add_item(
          items, item_count, capacity, &player->backpack[index]))
      return false;
  for (index = 0u; index < player->belt_count; ++index)
    if (!sf_saved_player_add_item(
          items, item_count, capacity, &player->belt[index]))
      return false;
  for (index = 0u; index < player->special_item_count; ++index)
    if (!sf_saved_player_add_item(
          items, item_count, capacity, &player->special_items[index]))
      return false;
  return true;
}
