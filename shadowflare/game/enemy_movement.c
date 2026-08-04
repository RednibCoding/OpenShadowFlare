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

#include "game/enemy_movement.h"

#include "core/retail_random.h"
#include "game/movement.h"
#include "game/scenario_enemy_controller.h"

#include <limits.h>

#define SF_ENEMY_PATROL_ACTION 1
#define SF_ENEMY_APPROACH_ACTION 10
#define SF_ENEMY_IDLE_PRESENTATION 7u
#define SF_ENEMY_WALK_PRESENTATION 8u
#define SF_ENEMY_MOVEMENT_SPEED_PARAMETER 3u
#define SF_ENEMY_TARGET_REFRESH_PARAMETER 7u
#define SF_ENEMY_RANDOM_TURN_PARAMETER 8u
#define SF_ENEMY_MOVEMENT_SCALE_INDEX 54u

static const SfEnemyControllerTarget *sf_enemy_current_target(
    const SfScenarioEnemy *enemy,
    const SfScenarioEnemyControllerContext *context) {
  return enemy->movement_target == 1u
    ? &context->companion : &context->player;
}

void sf_enemy_movement_stop(SfScenarioEnemy *enemy) {
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

bool sf_enemy_movement_begin_patrol(
    SfScenarioEnemy *enemy, int32_t duration, uint32_t *random_state) {
  if (!sf_enemy_scaled_speed(
        enemy, enemy->selected_action->parameters[
          SF_ENEMY_MOVEMENT_SPEED_PARAMETER], &enemy->movement_speed) ||
      !sf_enemy_patrol_destination(enemy, random_state)) return false;
  enemy->movement_active = true;
  enemy->movement_counter = 0;
  enemy->movement_duration = duration;
  enemy->presentation_action = SF_ENEMY_WALK_PRESENTATION;
  enemy->animation_frame = 0u;
  sf_route_reset(&enemy->route);
  return true;
}

bool sf_enemy_movement_begin_approach(
    SfScenarioEnemy *enemy, SfWorldPoint target) {
  if (!sf_enemy_scaled_speed(
        enemy, enemy->selected_action->parameters[
          SF_ENEMY_MOVEMENT_SPEED_PARAMETER], &enemy->movement_speed))
    return false;
  enemy->movement_active = true;
  enemy->movement_counter = 0;
  enemy->movement_duration = -1;
  enemy->movement_destination = target;
  enemy->presentation_action = SF_ENEMY_WALK_PRESENTATION;
  enemy->animation_frame = 0u;
  sf_route_reset(&enemy->route);
  return true;
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
    sf_enemy_movement_stop(enemy);
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

void sf_enemy_movement_update(
    SfScenarioEnemy *enemy,
    const SfScenarioEnemyControllerContext *context) {
  SfRouteStep movement;
  if (!enemy->movement_active) return;
  if (enemy->current_action == SF_ENEMY_PATROL_ACTION) {
    if (enemy->movement_counter >= enemy->movement_duration) {
      sf_enemy_movement_stop(enemy);
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
    sf_enemy_movement_stop(enemy);
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
