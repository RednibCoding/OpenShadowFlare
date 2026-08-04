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

#include "core/retail_random.h"
#include "game/enemy_ai_selection.h"
#include "game/movement.h"

#include <limits.h>

#define SF_ENEMY_WAIT_ACTION 0
#define SF_ENEMY_PATROL_ACTION 1
#define SF_ENEMY_APPROACH_ACTION 10
#define SF_ENEMY_IDLE_PRESENTATION 7u
#define SF_ENEMY_WALK_PRESENTATION 8u
#define SF_ENEMY_WAIT_EVENT 11
#define SF_ENEMY_PATROL_EVENT 12
#define SF_ENEMY_APPROACH_EVENT 15
#define SF_ENEMY_ACTIVATION_DISTANCE 5000
#define SF_ENEMY_ACTION_DURATION_PARAMETER 1u
#define SF_ENEMY_MOVEMENT_SPEED_PARAMETER 3u
#define SF_ENEMY_PATROL_DURATION_PARAMETER 4u
#define SF_ENEMY_PATROL_IDLE_PARAMETER 5u
#define SF_ENEMY_TARGET_REFRESH_PARAMETER 7u
#define SF_ENEMY_RANDOM_TURN_PARAMETER 8u
#define SF_ENEMY_TARGET_ENABLED_CONDITION 3u
#define SF_ENEMY_MINIMUM_DISTANCE_CONDITION 4u
#define SF_ENEMY_MAXIMUM_DISTANCE_CONDITION 5u
#define SF_ENEMY_MOVEMENT_SCALE_INDEX 54u

static int32_t sf_enemy_target_distance(
    const SfScenarioEnemy *enemy, SfEnemyControllerTarget target) {
  return target.valid ? sf_movement_bounds_distance(
    enemy->position, enemy->judgement,
    target.position, target.judgement) : -1;
}

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

static const SfEnemyControllerTarget *sf_enemy_current_target(
    const SfScenarioEnemy *enemy,
    const SfScenarioEnemyControllerContext *context) {
  return enemy->movement_target == 1u
    ? &context->companion : &context->player;
}

static void sf_enemy_stop_movement(SfScenarioEnemy *enemy) {
  const bool was_walking = enemy->animation_chart != 0u;
  enemy->movement_active = false;
  enemy->movement_speed = 0;
  enemy->movement_counter = 0;
  enemy->movement_duration = 0;
  enemy->movement_destination = enemy->position;
  enemy->presentation_action = SF_ENEMY_IDLE_PRESENTATION;
  enemy->animation_chart = 0u;
  if (was_walking) enemy->animation_frame = 0u;
  sf_route_reset(&enemy->route);
}

static bool sf_enemy_scaled_speed(
    const SfScenarioEnemy *enemy, int32_t authored, int32_t *speed) {
  const int64_t value = (int64_t) authored *
    enemy->definition->post_ai_values[SF_ENEMY_MOVEMENT_SCALE_INDEX] / 1000;
  if (value <= 0 || value > INT32_MAX) return false;
  *speed = (int32_t) value;
  return true;
}

static bool sf_enemy_patrol_destination(
    SfScenarioEnemy *enemy, uint32_t *random_state) {
  const int64_t minimum_x =
    (int64_t) enemy->spawn_position.x + enemy->patrol_bounds.left;
  const int64_t maximum_x =
    (int64_t) enemy->spawn_position.x + enemy->patrol_bounds.right;
  const int64_t minimum_y =
    (int64_t) enemy->spawn_position.y + enemy->patrol_bounds.top;
  const int64_t maximum_y =
    (int64_t) enemy->spawn_position.y + enemy->patrol_bounds.bottom;
  const int64_t width = maximum_x - minimum_x + 1;
  const int64_t height = maximum_y - minimum_y + 1;
  if (minimum_x < INT32_MIN || maximum_x > INT32_MAX ||
      minimum_y < INT32_MIN || maximum_y > INT32_MAX ||
      width <= 0 || width > INT32_MAX || height <= 0 || height > INT32_MAX)
    return false;
  enemy->movement_destination.x = (int32_t) minimum_x +
    sf_retail_random_next(random_state) % (int32_t) width;
  enemy->movement_destination.y = (int32_t) minimum_y +
    sf_retail_random_next(random_state) % (int32_t) height;
  return true;
}

static void sf_enemy_rotate_destination(
    SfScenarioEnemy *enemy, SfWorldPoint target, int32_t angle_step) {
  static const int16_t arctangent[15] = {
    8192, 4836, 2555, 1297, 651, 326, 163, 81,
    41, 20, 10, 5, 3, 1, 1
  };
  int32_t cosine = 19898;
  int32_t sine = 0;
  int32_t angle = angle_step * 32768 / 3000;
  uint8_t shift;
  const int64_t dx = (int64_t) target.x - enemy->position.x;
  const int64_t dy = (int64_t) target.y - enemy->position.y;
  for (shift = 0u; shift < 15u; ++shift) {
    const int32_t old_cosine = cosine;
    const int32_t divisor = (int32_t) (1u << shift);
    const int32_t shifted_cosine = old_cosine / divisor;
    const int32_t shifted_sine = sine / divisor;
    if (angle >= 0) {
      cosine -= shifted_sine;
      sine += shifted_cosine;
      angle -= arctangent[shift];
    } else {
      cosine += shifted_sine;
      sine -= shifted_cosine;
      angle += arctangent[shift];
    }
  }
  enemy->movement_destination.x = enemy->position.x +
    (int32_t) ((dx * cosine - dy * sine) / 32768);
  enemy->movement_destination.y = enemy->position.y +
    (int32_t) ((dx * sine + dy * cosine) / 32768);
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
  if (entering) sf_enemy_stop_movement(enemy);
  enemy->event_number =
    enemy->selected_action->parameters[SF_ENEMY_ACTION_DURATION_PARAMETER] <=
      enemy->action_counter ? 0 : SF_ENEMY_WAIT_EVENT;
}

static void sf_enemy_update_patrol(
    SfScenarioEnemy *enemy, bool entering, uint32_t *random_state) {
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
    if (!sf_enemy_scaled_speed(
          enemy, enemy->selected_action->parameters[
            SF_ENEMY_MOVEMENT_SPEED_PARAMETER], &enemy->movement_speed) ||
        !sf_enemy_patrol_destination(enemy, random_state)) {
      enemy->event_number = 0;
      return;
    }
    enemy->movement_active = true;
    enemy->movement_counter = 0;
    enemy->movement_duration = movement_duration;
    enemy->presentation_action = SF_ENEMY_WALK_PRESENTATION;
    enemy->animation_frame = 0u;
    enemy->patrol_counter = 0;
    sf_route_reset(&enemy->route);
  }
  enemy->event_number =
    enemy->selected_action->parameters[SF_ENEMY_ACTION_DURATION_PARAMETER] <=
      enemy->action_counter ? 1 : SF_ENEMY_PATROL_EVENT;
}

static void sf_enemy_update_approach(
    SfScenarioEnemy *enemy, bool entering,
    const SfScenarioEnemyControllerContext *context,
    SfEnemyAiTargetDistances distances) {
  if (!entering &&
      enemy->presentation_action != SF_ENEMY_WALK_PRESENTATION) {
    enemy->event_number = SF_ENEMY_APPROACH_ACTION;
    return;
  }
  if (entering) {
    const SfEnemyControllerTarget *target = sf_enemy_action_target(
      enemy->selected_action, context, distances, &enemy->movement_target);
    if (!target || !sf_enemy_scaled_speed(
          enemy, enemy->selected_action->parameters[
            SF_ENEMY_MOVEMENT_SPEED_PARAMETER], &enemy->movement_speed)) {
      enemy->event_number = SF_ENEMY_APPROACH_ACTION;
      sf_enemy_stop_movement(enemy);
      return;
    }
    enemy->movement_active = true;
    enemy->movement_counter = 0;
    enemy->movement_duration = -1;
    enemy->movement_destination = target->position;
    enemy->presentation_action = SF_ENEMY_WALK_PRESENTATION;
    enemy->animation_frame = 0u;
    sf_route_reset(&enemy->route);
  }
  enemy->event_number =
    enemy->selected_action->parameters[SF_ENEMY_ACTION_DURATION_PARAMETER] <=
      enemy->action_counter ? SF_ENEMY_APPROACH_ACTION : SF_ENEMY_APPROACH_EVENT;
}

static void sf_enemy_update_action(
    SfScenarioEnemy *enemy,
    const SfScenarioEnemyControllerContext *context,
    SfEnemyAiTargetDistances distances) {
  const int32_t action = enemy->selected_action
    ? enemy->selected_action->action_number : -1;
  const bool entering = enemy->current_action != action;
  if (entering) {
    enemy->current_action = action;
    enemy->action_counter = 0;
  }
  if (action == SF_ENEMY_WAIT_ACTION) {
    sf_enemy_update_wait(enemy, entering);
  } else if (action == SF_ENEMY_PATROL_ACTION) {
    sf_enemy_update_patrol(enemy, entering, context->random_state);
  } else if (action == SF_ENEMY_APPROACH_ACTION) {
    sf_enemy_update_approach(enemy, entering, context, distances);
  } else {
    enemy->event_number = 0;
    enemy->selected_action = NULL;
    enemy->current_action = -1;
    sf_enemy_stop_movement(enemy);
    return;
  }
  ++enemy->action_counter;
  ++enemy->patrol_counter;
}

static void sf_enemy_refresh_approach(
    SfScenarioEnemy *enemy,
    const SfScenarioEnemyControllerContext *context) {
  const SfEnemyControllerTarget *target =
    sf_enemy_current_target(enemy, context);
  int32_t refresh;
  bool should_refresh;
  if (!target->valid || sf_movement_bounds_distance(
        enemy->position, enemy->judgement,
        target->position, target->judgement) <= 0) {
    sf_enemy_stop_movement(enemy);
    return;
  }
  refresh = enemy->selected_action->parameters[
    SF_ENEMY_TARGET_REFRESH_PARAMETER];
  if (refresh == 0) refresh = 1;
  should_refresh = enemy->movement_counter % refresh == 0 ||
    (enemy->movement_destination.x == enemy->position.x &&
     enemy->movement_destination.y == enemy->position.y);
  if (should_refresh) {
    const int32_t chance = enemy->selected_action->parameters[
      SF_ENEMY_RANDOM_TURN_PARAMETER];
    enemy->movement_destination = target->position;
    if (sf_retail_random_next(context->random_state) % 100 < chance) {
      const int32_t angle_step =
        sf_retail_random_next(context->random_state) % 2001 - 1000;
      sf_enemy_rotate_destination(enemy, target->position, angle_step);
    }
    sf_route_reset(&enemy->route);
  }
  ++enemy->movement_counter;
}

static void sf_enemy_update_movement(
    SfScenarioEnemy *enemy,
    const SfScenarioEnemyControllerContext *context) {
  SfRouteStep movement;
  if (!enemy->movement_active) return;
  if (enemy->current_action == SF_ENEMY_PATROL_ACTION) {
    if (enemy->movement_counter >= enemy->movement_duration) {
      sf_enemy_stop_movement(enemy);
      return;
    }
    ++enemy->movement_counter;
  } else if (enemy->current_action == SF_ENEMY_APPROACH_ACTION) {
    sf_enemy_refresh_approach(enemy, context);
    if (!enemy->movement_active) return;
  }
  movement = sf_route_advance_query(
    &enemy->route, context->collision, enemy->judgement,
    enemy->position, enemy->movement_destination,
    (uint32_t) enemy->movement_speed);
  if (movement.moved)
    enemy->direction = sf_movement_direction(
      enemy->position, movement.position);
  enemy->position = movement.position;
  if (!movement.moved && !movement.controller_active) {
    sf_enemy_stop_movement(enemy);
    return;
  }
  if (movement.moved) {
    if (enemy->animation_chart != 1u) {
      enemy->animation_chart = 1u;
      enemy->animation_frame = 0u;
    } else {
      ++enemy->animation_frame;
    }
  } else {
    if (enemy->animation_chart != 0u) {
      enemy->animation_chart = 0u;
      enemy->animation_frame = 0u;
    } else {
      ++enemy->animation_frame;
    }
  }
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
    sf_enemy_stop_movement(enemy);
    ++enemy->animation_frame;
    return;
  }
  sf_enemy_select_action(enemy, context, distances);
  sf_enemy_update_action(enemy, context, distances);
  sf_enemy_update_movement(enemy, context);
  if (!enemy->movement_active && enemy->animation_chart == 0u)
    ++enemy->animation_frame;
}
