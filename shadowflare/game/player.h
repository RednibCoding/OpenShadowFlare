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

#ifndef SHADOWFLARE_GAME_PLAYER_H
#define SHADOWFLARE_GAME_PLAYER_H

#include "core/coordinates.h"
#include "data/item.h"
#include "data/player_parameters.h"
#include "game/route.h"
#include "game/inventory.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_PLAYER_APPEARANCE_PART_LIMIT 8u
#define SF_PLAYER_VISIBLE_ITEM_LIMIT 3u

typedef enum SfPlayerMotion {
  SF_PLAYER_IDLE = 0,
  SF_PLAYER_WALKING = 1,
  SF_PLAYER_RUNNING = 2
} SfPlayerMotion;

typedef enum SfPlayerPace {
  SF_PLAYER_PACE_WALK = 0,
  SF_PLAYER_PACE_RUN
} SfPlayerPace;

typedef struct SfPlayerState {
  SfWorldPoint position;
  SfWorldPoint previous_position;
  SfWorldPoint destination;
  SfObjectBounds judgement;
  SfRouteController route;
  SfInventoryState inventory;
  SfPlayerInitialParameters initial_parameters;
  int32_t current_life;
  int32_t current_mana;
  int32_t experience;
  int32_t level;
  uint32_t action_counter;
  uint32_t animation_frame;
  uint8_t appearance_parts[SF_PLAYER_APPEARANCE_PART_LIMIT];
  SfItemReference visible_items[SF_PLAYER_VISIBLE_ITEM_LIMIT];
  uint8_t direction;
  uint8_t gender;
  uint8_t appearance_part_count;
  uint8_t visible_item_count;
  uint8_t walking_speed_tier;
  SfPlayerMotion motion;
  SfPlayerMotion previous_motion;
  SfPlayerPace pace;
  bool parameters_initialized;
} SfPlayerState;

void sf_player_init(SfPlayerState *player, uint8_t gender);
bool sf_player_apply_initial_parameters(
  SfPlayerState *player, const SfPlayerInitialParameters *parameters);
void sf_player_enter(
  SfPlayerState *player, SfWorldPoint position, uint8_t direction);
void sf_player_move_to(SfPlayerState *player, SfWorldPoint destination);
void sf_player_follow_to(SfPlayerState *player, SfWorldPoint destination);
void sf_player_cancel_movement(SfPlayerState *player);
void sf_player_toggle_pace(SfPlayerState *player);
SfWorldPoint sf_player_render_position(
  const SfPlayerState *player, uint16_t interpolation);
void sf_player_update(
  SfPlayerState *player, const SfCollisionWorld *collision);
void sf_player_update_query(
  SfPlayerState *player, const SfCollisionQuery *collision);

#endif
