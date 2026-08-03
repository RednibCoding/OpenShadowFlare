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

#include "game/input.h"
#include "game/player.h"
#include "game/scenario_actor.h"
#include "interpreter/scenario_actor_script.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_WORLD_MOVEMENT_BLOCKER_LIMIT (SF_MCT_PERSON_LIMIT + 1u)

typedef struct SfWorldPointerControl {
  int32_t hovered_actor_id;
  int32_t pending_actor_id;
  int16_t screen_x;
  int16_t screen_y;
  uint8_t hold_updates;
  uint8_t range;
  bool active;
  bool range_enabled;
  bool interaction_command_active;
  bool ground_command_active;
  bool continuous_movement;
  bool previous_down;
} SfWorldPointerControl;

typedef struct SfWorldRenderView {
  SfWorldPoint player_position;
  int32_t camera_x;
  int32_t camera_y;
} SfWorldRenderView;

typedef struct SfWorldState {
  int32_t scenario_id;
  int32_t entry_key;
  int32_t camera_x;
  int32_t camera_y;
  SfPlayerState player;
  SfScenarioActorSet actors;
  SfScenarioActorScriptState actor_script_state;
  SfCollisionWorld collision;
  SfMovementBlocker movement_blockers[SF_WORLD_MOVEMENT_BLOCKER_LIMIT];
  const SfScsScript *script;
  SfWorldPointerControl pointer;
  int32_t companion_type;
  uint8_t movement_blocker_count;
  bool entered;
} SfWorldState;

void sf_world_state_init(
  SfWorldState *world, int32_t scenario_id, int32_t entry_key,
  uint8_t player_gender);
void sf_world_state_enter(
  SfWorldState *world,
  int32_t player_x, int32_t player_y, uint8_t direction);
void sf_world_state_bind_collision(
  SfWorldState *world,
  const SfGroundMap *ground, const SfObjectMap *objects);
bool sf_world_state_bind_scenario(
  SfWorldState *world,
  const SfMctScenario *scenario, const SfScsScript *script);
void sf_world_state_update(SfWorldState *world, const SfGameInput *input);
void sf_world_render_view(
  const SfWorldState *world, uint16_t interpolation,
  SfWorldRenderView *view);

#endif
