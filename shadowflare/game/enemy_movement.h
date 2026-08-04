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

#ifndef SHADOWFLARE_GAME_ENEMY_MOVEMENT_H
#define SHADOWFLARE_GAME_ENEMY_MOVEMENT_H

#include "game/scenario_enemy.h"

#include <stdbool.h>
#include <stdint.h>

struct SfScenarioEnemyControllerContext;

void sf_enemy_movement_stop(SfScenarioEnemy *enemy);
bool sf_enemy_movement_begin_patrol(
  SfScenarioEnemy *enemy, int32_t duration, uint32_t *random_state);
bool sf_enemy_movement_begin_approach(
  SfScenarioEnemy *enemy, SfWorldPoint target);
void sf_enemy_movement_update(
  SfScenarioEnemy *enemy,
  const struct SfScenarioEnemyControllerContext *context);

#endif
