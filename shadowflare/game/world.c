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

#include "game/world.h"

#include "core/coordinates.h"

#include <string.h>

void sf_world_state_init(
    SfWorldState *world, int32_t scenario_id, int32_t entry_key) {
  if (!world) return;
  memset(world, 0, sizeof(*world));
  world->scenario_id = scenario_id;
  world->entry_key = entry_key;
}

void sf_world_state_enter(
    SfWorldState *world,
    int32_t player_x, int32_t player_y, uint8_t direction) {
  SfScreenPoint screen;
  if (!world || direction > 7u) return;
  world->player_x = player_x;
  world->player_y = player_y;
  world->player_direction = direction;
  screen = sf_world_to_screen((SfWorldPoint) {player_x, player_y});
  world->camera_x = screen.x - 320;
  world->camera_y = screen.y - 240;
  world->entered = true;
}
