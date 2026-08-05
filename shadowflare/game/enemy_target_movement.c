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

#include "game/enemy_target_movement.h"

#include "core/retail_random.h"
#include "game/enemy_movement.h"
#include "game/movement.h"
#include "game/scenario_enemy_controller.h"

#define SF_ENEMY_RETREAT_ACTION 9
#define SF_ENEMY_TARGET_REFRESH_PARAMETER 7u
#define SF_ENEMY_RANDOM_TURN_PARAMETER 8u
#define SF_ENEMY_RETREAT_DISTANCE 10000

static const SfEnemyControllerTarget *sf_enemy_current_target(
    const SfScenarioEnemy *enemy,
    const SfScenarioEnemyControllerContext *context) {
  return enemy->movement_target == 1u
    ? &context->companion : &context->player;
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

static bool sf_enemy_retreat_destination(
    SfScenarioEnemy *enemy, SfWorldPoint target) {
  return sf_movement_point_at_distance(
    target, enemy->position, SF_ENEMY_RETREAT_DISTANCE + 1,
    &enemy->movement_destination);
}

bool sf_enemy_target_movement_refresh(
    SfScenarioEnemy *enemy,
    const SfScenarioEnemyControllerContext *context) {
  const SfEnemyControllerTarget *target =
    sf_enemy_current_target(enemy, context);
  const bool retreat = enemy->current_action == SF_ENEMY_RETREAT_ACTION;
  int32_t distance;
  int32_t refresh;
  bool should_refresh;
  if (!target->valid) {
    sf_enemy_movement_stop(enemy);
    return false;
  }
  distance = sf_movement_bounds_distance(
    enemy->position, enemy->judgement,
    target->position, target->judgement);
  if ((!retreat && distance <= 0) ||
      (retreat && distance >= SF_ENEMY_RETREAT_DISTANCE)) {
    sf_enemy_movement_stop(enemy);
    return false;
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
    if (retreat) {
      if (!sf_enemy_retreat_destination(enemy, target->position)) {
        sf_enemy_movement_stop(enemy);
        return false;
      }
    } else {
      enemy->movement_destination = target->position;
    }
    if (sf_retail_random_next(context->random_state) % 100 < chance) {
      const int32_t angle_step =
        sf_retail_random_next(context->random_state) % 2001 - 1000;
      sf_enemy_rotate_destination(
        enemy, enemy->movement_destination, angle_step);
    }
    sf_route_reset(&enemy->route);
  }
  ++enemy->movement_counter;
  /* Retail resolves companion retreat once, but does not take a move step. */
  if (retreat && enemy->movement_target == 1u) {
    sf_enemy_movement_stop(enemy);
    return false;
  }
  return true;
}
