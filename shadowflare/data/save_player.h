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

#ifndef SHADOWFLARE_DATA_SAVE_PLAYER_H
#define SHADOWFLARE_DATA_SAVE_PLAYER_H

#include "data/item.h"
#include "data/player_parameters.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_SAVED_PLAYER_NAME_CAPACITY 25u
#define SF_SAVED_PLAYER_RECORD_SIZE 0x160u
#define SF_SAVED_EQUIPMENT_COUNT 11u
#define SF_SAVED_BACKPACK_ITEM_LIMIT 36u
#define SF_SAVED_BELT_ITEM_LIMIT 8u
#define SF_SAVED_SPECIAL_ITEM_LIMIT 90u

typedef struct SfSavedItem {
  int32_t definition_id;
  int32_t quantity;
  int32_t durability;
  int32_t grid_x;
  int32_t grid_y;
  uint8_t category;
  bool identified;
  bool present;
} SfSavedItem;

typedef struct SfSavedPlayer {
  char name[SF_SAVED_PLAYER_NAME_CAPACITY];
  SfPlayerInitialParameters parameters;
  SfSavedItem equipment[SF_SAVED_EQUIPMENT_COUNT];
  SfSavedItem backpack[SF_SAVED_BACKPACK_ITEM_LIMIT];
  SfSavedItem belt[SF_SAVED_BELT_ITEM_LIMIT];
  SfSavedItem special_items[SF_SAVED_SPECIAL_ITEM_LIMIT];
  int32_t gender;
  int32_t job;
  int32_t level;
  int32_t current_life;
  int32_t current_mana;
  int32_t experience;
  uint8_t backpack_count;
  uint8_t belt_count;
  uint8_t special_item_count;
} SfSavedPlayer;

bool sf_saved_player_required_items(
  const SfSavedPlayer *player, SfItemReference *items,
  uint8_t capacity, uint8_t *item_count);

#endif
