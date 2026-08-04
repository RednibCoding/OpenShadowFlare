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

#include "game/enemy_action.h"

#include "game/enemy_movement.h"
#include "game/enemy_presentation.h"
#include "game/scenario_enemy_controller.h"

#include <limits.h>

#define SF_ENEMY_WAIT_ACTION 0
#define SF_ENEMY_PATROL_ACTION 1
#define SF_ENEMY_RETREAT_ACTION 9
#define SF_ENEMY_APPROACH_ACTION 10
#define SF_ENEMY_DIRECT_ATTACK_ACTION 2
#define SF_ENEMY_IDLE_PRESENTATION 7u
#define SF_ENEMY_WALK_PRESENTATION 8u
#define SF_ENEMY_WAIT_EVENT 11
#define SF_ENEMY_PATROL_EVENT 12
#define SF_ENEMY_RETREAT_EVENT 14
#define SF_ENEMY_APPROACH_EVENT 15
#define SF_ENEMY_ACTION_DURATION_PARAMETER 1u
#define SF_ENEMY_PATROL_DURATION_PARAMETER 4u
#define SF_ENEMY_PATROL_IDLE_PARAMETER 5u
#define SF_ENEMY_TARGET_ENABLED_CONDITION 3u
#define SF_ENEMY_MINIMUM_DISTANCE_CONDITION 4u
#define SF_ENEMY_MAXIMUM_DISTANCE_CONDITION 5u

static bool sf_enemy_distance_in_range(
    int32_t distance, int32_t minimum, int32_t maximum) {
  return distance >= 0 && (minimum == -1 || distance >= minimum) &&
    (maximum == -1 || distance <= maximum);
}

static const SfEnemyControllerTarget *sf_enemy_action_target(
    const SfAiAction *action,
    const SfScenarioEnemyControllerContext *context,
    SfEnemyAiTargetDistances distances, uint8_t *target_index) {
  if (action->conditions[SF_ENEMY_TARGET_ENABLED_CONDITION] == 1) {
    const int32_t minimum =
      action->conditions[SF_ENEMY_MINIMUM_DISTANCE_CONDITION];
    const int32_t maximum =
      action->conditions[SF_ENEMY_MAXIMUM_DISTANCE_CONDITION];
    if (context->player.valid && sf_enemy_distance_in_range(
          distances.player, minimum, maximum)) {
      *target_index = 0u;
      return &context->player;
    }
    if (context->companion.valid && sf_enemy_distance_in_range(
          distances.companion, minimum, maximum)) {
      *target_index = 1u;
      return &context->companion;
    }
    return NULL;
  }
  if (context->player.valid) {
    *target_index = 0u;
    return &context->player;
  }
  if (context->companion.valid) {
    *target_index = 1u;
    return &context->companion;
  }
  return NULL;
}

static void sf_enemy_select_action(
    SfScenarioEnemy *enemy,
    const SfScenarioEnemyControllerContext *context,
    SfEnemyAiTargetDistances distances) {
  const SfAiAction *selected;
  if (enemy->event_number < 0) return;
  selected = sf_enemy_ai_select(
    context->catalog, enemy->control, enemy->event_number,
    enemy->current_life, enemy->maximum_life,
    distances, context->random_state);
  if (!selected) return;
  enemy->selected_action = selected;
  enemy->current_action = -1;
  enemy->event_number = -1;
}

static void sf_enemy_update_wait(SfScenarioEnemy *enemy, bool entering) {
  if (entering) sf_enemy_movement_stop(enemy);
  enemy->event_number =
    enemy->selected_action->parameters[SF_ENEMY_ACTION_DURATION_PARAMETER] <=
      enemy->action_counter ? 0 : SF_ENEMY_WAIT_EVENT;
}

static void sf_enemy_update_patrol(
    SfScenarioEnemy *enemy, bool entering,
    const SfScenarioEnemyControllerContext *context) {
  const int32_t movement_duration = enemy->selected_action->parameters[
    SF_ENEMY_PATROL_DURATION_PARAMETER];
  const int32_t idle_duration = enemy->selected_action->parameters[
    SF_ENEMY_PATROL_IDLE_PARAMETER];
  const int64_t cycle_value = (int64_t) movement_duration + idle_duration;
  int32_t cycle;
  if (movement_duration < 0 || idle_duration < 0 ||
      cycle_value > INT32_MAX) {
    enemy->event_number = 0;
    return;
  }
  cycle = (int32_t) cycle_value;
  if (entering) {
    enemy->patrol_counter = cycle;
  } else if (enemy->presentation_action != SF_ENEMY_WALK_PRESENTATION &&
      enemy->patrol_counter < movement_duration) {
    enemy->patrol_counter = movement_duration;
  }
  if (enemy->patrol_counter == cycle) {
    if (!sf_enemy_movement_begin_patrol(
          enemy, movement_duration, context->random_state)) {
      enemy->event_number = 0;
      return;
    }
    enemy->patrol_counter = 0;
  }
  enemy->event_number =
    enemy->selected_action->parameters[SF_ENEMY_ACTION_DURATION_PARAMETER] <=
      enemy->action_counter ? 1 : SF_ENEMY_PATROL_EVENT;
}

static void sf_enemy_update_target_movement(
    SfScenarioEnemy *enemy, bool entering, int32_t holding_event,
    const SfScenarioEnemyControllerContext *context,
    SfEnemyAiTargetDistances distances) {
  if (!entering &&
      enemy->presentation_action != SF_ENEMY_WALK_PRESENTATION) {
    enemy->event_number = enemy->current_action;
    return;
  }
  if (entering) {
    const SfEnemyControllerTarget *target = sf_enemy_action_target(
      enemy->selected_action, context, distances, &enemy->movement_target);
    if (!target || !sf_enemy_movement_begin_target(enemy, target->position)) {
      enemy->event_number = enemy->current_action;
      sf_enemy_movement_stop(enemy);
      return;
    }
  }
  enemy->event_number =
    enemy->selected_action->parameters[SF_ENEMY_ACTION_DURATION_PARAMETER] <=
      enemy->action_counter
      ? enemy->current_action : holding_event;
}

static void sf_enemy_update_direct_attack(
    SfScenarioEnemy *enemy,
    const SfScenarioEnemyControllerContext *context) {
  if (enemy->presentation_action == 1u) return;
  if (!enemy->direct_attack_animations) {
    if (context->attack_request && context->attack_request->resource_id < 0) {
      context->attack_request->resource_id = enemy->definition->resource_id;
      context->attack_request->chart =
        enemy->definition->post_ai_values[41] + 4;
    }
    return;
  }
  sf_enemy_movement_stop(enemy);
  if (!sf_enemy_presentation_begin_direct(enemy, context))
    enemy->event_number = SF_ENEMY_DIRECT_ATTACK_ACTION;
}

void sf_enemy_action_update(
    SfScenarioEnemy *enemy,
    const SfScenarioEnemyControllerContext *context,
    SfEnemyAiTargetDistances distances) {
  int32_t action;
  bool entering;
  sf_enemy_select_action(enemy, context, distances);
  action = enemy->selected_action
    ? enemy->selected_action->action_number : -1;
  entering = enemy->current_action != action;
  if (entering) {
    enemy->current_action = action;
    enemy->action_counter = 0;
  }
  if (action == SF_ENEMY_WAIT_ACTION) {
    sf_enemy_update_wait(enemy, entering);
  } else if (action == SF_ENEMY_PATROL_ACTION) {
    sf_enemy_update_patrol(enemy, entering, context);
  } else if (action == SF_ENEMY_RETREAT_ACTION) {
    sf_enemy_update_target_movement(
      enemy, entering, SF_ENEMY_RETREAT_EVENT, context, distances);
  } else if (action == SF_ENEMY_APPROACH_ACTION) {
    sf_enemy_update_target_movement(
      enemy, entering, SF_ENEMY_APPROACH_EVENT, context, distances);
  } else if (action == SF_ENEMY_DIRECT_ATTACK_ACTION) {
    sf_enemy_update_direct_attack(enemy, context);
  } else {
    enemy->event_number = 0;
    enemy->selected_action = NULL;
    enemy->current_action = -1;
    sf_enemy_movement_stop(enemy);
    return;
  }
  ++enemy->action_counter;
  ++enemy->patrol_counter;
}
