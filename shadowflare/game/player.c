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

#include "game/player.h"

#include "game/movement.h"

#include <string.h>

static uint32_t sf_player_speed(const SfPlayerState *player) {
  static const uint8_t speeds[10] = {13u, 15u, 16u, 16u, 17u,
                                    20u, 23u, 24u, 25u, 26u};
  const uint8_t tier = player->walking_speed_tier > 9u
    ? 9u : player->walking_speed_tier;
  return player->pace == SF_PLAYER_PACE_RUN
    ? (uint32_t) speeds[tier] * 2u : speeds[tier];
}

void sf_player_init(SfPlayerState *player, uint8_t gender) {
  if (!player) return;
  memset(player, 0, sizeof(*player));
  player->gender = gender == 1u ? 1u : 0u;
  player->appearance_parts[0] = 0u;
  player->appearance_parts[1] = 1u;
  player->appearance_part_count = 2u;
  player->visible_items[0].category = 1u;
  player->visible_items[0].definition_id = 0u;
  player->visible_item_count = 1u;
  player->judgement = (SfObjectBounds) {-80, -80, 79, 79};
  player->direction = 1u;
  player->walking_speed_tier = 5u;
  player->level = 1;
  player->current_life = 1;
  player->current_mana = 1;
  player->maximum_mines = 10;
  player->motion = SF_PLAYER_IDLE;
  player->previous_motion = SF_PLAYER_IDLE;
  sf_inventory_init(&player->inventory);
  sf_equipment_init(&player->equipment);
  sf_belt_init(&player->belt);
  sf_special_items_init(&player->special_items);
  sf_player_magic_init(&player->magic);
  sf_route_reset(&player->route);
}

void sf_player_set_identity(
    SfPlayerState *player, const char *name, int32_t job) {
  size_t length = 0u;
  if (!player) return;
  memset(player->name, 0, sizeof(player->name));
  if (name) {
    while (length + 1u < sizeof(player->name) && name[length] != '\0')
      ++length;
    memcpy(player->name, name, length);
  }
  player->job = job;
}

bool sf_player_apply_initial_parameters(
    SfPlayerState *player, const SfPlayerInitialParameters *parameters) {
  int32_t speed_tier;
  if (!player || !parameters || parameters->values[2] <= 0 ||
      parameters->values[3] <= 0 ||
      parameters->experience_threshold <= 0) return false;
  player->initial_parameters = *parameters;
  player->level = 1;
  player->current_life = parameters->values[2];
  player->current_mana = parameters->values[3];
  player->experience = 0;
  speed_tier = (parameters->values[1] + 32) / 32;
  if (speed_tier < 0) speed_tier = 0;
  if (speed_tier > 9) speed_tier = 9;
  player->walking_speed_tier = (uint8_t) speed_tier;
  player->parameters_initialized = true;
  return true;
}

void sf_player_enter(
    SfPlayerState *player, SfWorldPoint position, uint8_t direction) {
  if (!player) return;
  player->position = position;
  player->previous_position = position;
  player->destination = position;
  player->direction = direction < 8u ? direction : 1u;
  player->action_counter = 0u;
  player->animation_frame = 0u;
  player->motion = SF_PLAYER_IDLE;
  player->previous_motion = SF_PLAYER_IDLE;
  sf_route_reset(&player->route);
}

static void sf_player_start_movement(
    SfPlayerState *player, SfWorldPoint destination) {
  if (!player) return;
  player->destination = destination;
  player->motion = player->pace == SF_PLAYER_PACE_RUN
    ? SF_PLAYER_RUNNING : SF_PLAYER_WALKING;
}

void sf_player_move_to(SfPlayerState *player, SfWorldPoint destination) {
  sf_player_start_movement(player, destination);
}

void sf_player_follow_to(SfPlayerState *player, SfWorldPoint destination) {
  sf_player_start_movement(player, destination);
}

void sf_player_cancel_movement(SfPlayerState *player) {
  if (!player) return;
  player->destination = player->position;
  player->motion = SF_PLAYER_IDLE;
  sf_route_reset(&player->route);
}

void sf_player_toggle_pace(SfPlayerState *player) {
  if (!player) return;
  player->pace = player->pace == SF_PLAYER_PACE_WALK
    ? SF_PLAYER_PACE_RUN : SF_PLAYER_PACE_WALK;
  if (player->motion == SF_PLAYER_WALKING ||
      player->motion == SF_PLAYER_RUNNING)
    player->motion = player->pace == SF_PLAYER_PACE_RUN
      ? SF_PLAYER_RUNNING : SF_PLAYER_WALKING;
}

SfWorldPoint sf_player_render_position(
    const SfPlayerState *player, uint16_t interpolation) {
  SfWorldPoint result = {0, 0};
  if (!player) return result;
  return sf_world_point_interpolate(
    player->previous_position, player->position, interpolation);
}

void sf_player_update_query(
    SfPlayerState *player, const SfCollisionQuery *collision) {
  SfRouteStep movement;
  if (!player) return;
  player->previous_position = player->position;
  if (player->motion == SF_PLAYER_IDLE) {
    if (player->previous_motion != SF_PLAYER_IDLE) player->action_counter = 0u;
    player->animation_frame = player->action_counter++;
    player->previous_motion = SF_PLAYER_IDLE;
    return;
  }
  if (player->previous_motion != player->motion) player->action_counter = 0u;
  else ++player->action_counter;
  player->animation_frame = player->action_counter;
  player->previous_motion = player->motion;
  if (player->position.x == player->destination.x &&
      player->position.y == player->destination.y) {
    player->motion = SF_PLAYER_IDLE;
    return;
  }
  player->direction = sf_movement_direction(
    player->position, player->destination);
  movement = sf_route_advance_query(
    &player->route, collision, player->judgement,
    player->position, player->destination, sf_player_speed(player));
  if (movement.moved)
    player->direction = sf_movement_direction(
      player->position, movement.position);
  player->position = movement.position;
  if (!movement.moved && !movement.controller_active)
    player->motion = SF_PLAYER_IDLE;
}

void sf_player_update(
    SfPlayerState *player, const SfCollisionWorld *collision) {
  const SfCollisionQuery query = {collision, NULL, INT32_MIN, 0u};
  sf_player_update_query(player, &query);
}
