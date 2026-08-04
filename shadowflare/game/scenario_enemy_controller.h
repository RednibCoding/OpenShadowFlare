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

#ifndef SHADOWFLARE_GAME_SCENARIO_ENEMY_CONTROLLER_H
#define SHADOWFLARE_GAME_SCENARIO_ENEMY_CONTROLLER_H

#include "data/ai_control.h"
#include "game/collision.h"
#include "game/scenario_enemy.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct SfEnemyControllerTarget {
  SfWorldPoint position;
  SfObjectBounds judgement;
  bool valid;
} SfEnemyControllerTarget;

typedef struct SfScenarioEnemyControllerContext {
  const SfAiControlCatalog *catalog;
  const SfCollisionQuery *collision;
  SfEnemyControllerTarget player;
  SfEnemyControllerTarget companion;
  uint32_t *random_state;
} SfScenarioEnemyControllerContext;

void sf_scenario_enemy_controller_update(
  SfScenarioEnemy *enemy,
  const SfScenarioEnemyControllerContext *context);

#endif
