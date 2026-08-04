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

#include "game/scenario_enemy_controller.h"

#include "game/enemy_action.h"
#include "game/enemy_movement.h"
#include "game/movement.h"

#define SF_ENEMY_ACTIVATION_DISTANCE 5000

static int32_t sf_enemy_target_distance(
    const SfScenarioEnemy *enemy, SfEnemyControllerTarget target) {
  return target.valid ? sf_movement_bounds_distance(
    enemy->position, enemy->judgement,
    target.position, target.judgement) : -1;
}

void sf_scenario_enemy_controller_update(
    SfScenarioEnemy *enemy,
    const SfScenarioEnemyControllerContext *context) {
  SfEnemyAiTargetDistances distances;
  bool active;
  if (!enemy || !enemy->definition || !enemy->control || !context ||
      !context->catalog || !context->collision || !context->random_state)
    return;
  enemy->previous_position = enemy->position;
  if (enemy->current_life <= 0 ||
      !sf_scenario_enemy_state(enemy, SF_SCENARIO_VISIBLE)) return;
  distances.player = sf_enemy_target_distance(enemy, context->player);
  distances.companion = sf_enemy_target_distance(enemy, context->companion);
  active = (distances.player >= 0 &&
      distances.player <= SF_ENEMY_ACTIVATION_DISTANCE) ||
    (distances.companion >= 0 &&
      distances.companion <= SF_ENEMY_ACTIVATION_DISTANCE);
  if (!active) {
    enemy->selected_action = NULL;
    enemy->current_action = -1;
    enemy->event_number = 0;
    sf_enemy_movement_stop(enemy);
    ++enemy->animation_frame;
    return;
  }
  sf_enemy_action_update(enemy, context, distances);
  sf_enemy_movement_update(enemy, context);
  if (!enemy->movement_active && enemy->animation_chart == 0u)
    ++enemy->animation_frame;
}
