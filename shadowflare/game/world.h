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

#ifndef SHADOWFLARE_GAME_WORLD_H
#define SHADOWFLARE_GAME_WORLD_H

#include "data/item.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_PLAYER_APPEARANCE_PART_LIMIT 8u
#define SF_PLAYER_VISIBLE_ITEM_LIMIT 3u

typedef struct SfWorldState {
  int32_t scenario_id;
  int32_t entry_key;
  int32_t player_x;
  int32_t player_y;
  int32_t camera_x;
  int32_t camera_y;
  uint32_t player_animation_frame;
  uint8_t player_appearance_parts[SF_PLAYER_APPEARANCE_PART_LIMIT];
  SfItemReference player_visible_items[SF_PLAYER_VISIBLE_ITEM_LIMIT];
  uint8_t player_direction;
  uint8_t player_gender;
  uint8_t player_appearance_part_count;
  uint8_t player_visible_item_count;
  bool entered;
} SfWorldState;

void sf_world_state_init(
  SfWorldState *world, int32_t scenario_id, int32_t entry_key,
  uint8_t player_gender);
void sf_world_state_enter(
  SfWorldState *world,
  int32_t player_x, int32_t player_y, uint8_t direction);
void sf_world_state_update(SfWorldState *world);

#endif
