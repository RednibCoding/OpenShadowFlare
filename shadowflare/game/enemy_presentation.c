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

#include "game/enemy_presentation.h"

#include "game/movement.h"

#include <limits.h>

#define SF_ENEMY_DIRECT_PRESENTATION 1u
#define SF_ENEMY_IDLE_PRESENTATION 7u
#define SF_ENEMY_IMPACT_MARKER 0x40
#define SF_ENEMY_DIRECT_RANGE_INDEX 3u
#define SF_ENEMY_DIRECT_CHART_INDEX 41u
#define SF_ENEMY_DIRECT_SPEED_INDEX 47u

static const uint8_t sf_enemy_speed_numerator[10] = {
  3u, 2u, 3u, 4u, 1u, 3u, 2u, 5u, 3u, 4u};
static const uint8_t sf_enemy_speed_denominator[10] = {
  10u, 5u, 5u, 5u, 1u, 2u, 1u, 2u, 1u, 1u};

static int32_t sf_enemy_target_distance(
    const SfScenarioEnemy *enemy, SfEnemyControllerTarget target) {
  return target.valid ? sf_movement_bounds_distance(
    enemy->position, enemy->judgement,
    target.position, target.judgement) : -1;
}

static const SfEnemyControllerTarget *sf_enemy_direct_target(
    const SfScenarioEnemy *enemy,
    const SfScenarioEnemyControllerContext *context, uint8_t *target_index) {
  const int32_t maximum =
    enemy->definition->post_ai_values[SF_ENEMY_DIRECT_RANGE_INDEX];
  const int32_t player_distance = sf_enemy_target_distance(
    enemy, context->player);
  const int32_t companion_distance = sf_enemy_target_distance(
    enemy, context->companion);
  if (player_distance >= 0 && player_distance <= maximum) {
    *target_index = 0u;
    return &context->player;
  }
  if (companion_distance >= 0 && companion_distance <= maximum) {
    *target_index = 1u;
    return &context->companion;
  }
  return NULL;
}

void sf_enemy_presentation_reset(SfScenarioEnemy *enemy) {
  if (!enemy) return;
  enemy->presentation_action = SF_ENEMY_IDLE_PRESENTATION;
  enemy->animation_chart = 0u;
  enemy->animation_frame = 0u;
  enemy->presentation_elapsed = 0;
  enemy->presentation_previous_frame = -1;
  enemy->presentation_target = UINT8_MAX;
  enemy->presentation_audio_markers = 0u;
  enemy->direct_impact_pending = false;
}

static void sf_enemy_presentation_finish(SfScenarioEnemy *enemy) {
  enemy->presentation_action = SF_ENEMY_IDLE_PRESENTATION;
  enemy->animation_chart = 0u;
  enemy->animation_frame = 0u;
  enemy->presentation_elapsed = 0;
  enemy->presentation_previous_frame = -1;
  enemy->presentation_target = UINT8_MAX;
}

bool sf_enemy_presentation_begin_direct(
    SfScenarioEnemy *enemy,
    const SfScenarioEnemyControllerContext *context) {
  const SfEnemyControllerTarget *target;
  int32_t chart;
  if (!enemy || !enemy->definition || !context ||
      !enemy->direct_attack_animations) return false;
  chart = enemy->definition->post_ai_values[SF_ENEMY_DIRECT_CHART_INDEX] + 4;
  if (chart < 0 || chart > UINT8_MAX) return false;
  enemy->presentation_action = SF_ENEMY_DIRECT_PRESENTATION;
  enemy->animation_chart = (uint8_t) chart;
  enemy->animation_frame = 0u;
  enemy->presentation_elapsed = 0;
  enemy->presentation_previous_frame = -1;
  enemy->presentation_target = UINT8_MAX;
  target = sf_enemy_direct_target(
    enemy, context, &enemy->presentation_target);
  if (target)
    enemy->direction = sf_movement_direction(enemy->position, target->position);
  return true;
}

static uint8_t sf_enemy_audio_markers(int16_t status) {
  uint8_t markers = 0u;
  if ((status & 0x400) != 0) markers |= 1u;
  if ((status & 0x800) != 0) markers |= 2u;
  if ((status & 0x1000) != 0) markers |= 4u;
  return markers;
}

void sf_enemy_presentation_update(
    SfScenarioEnemy *enemy,
    const SfScenarioEnemyControllerContext *context) {
  const SfCafSelectedAnimation *animation;
  int32_t speed_index;
  int32_t frame;
  int32_t scan;
  if (!enemy || !context || enemy->presentation_action != 1u) return;
  enemy->presentation_audio_markers = 0u;
  enemy->direct_impact_pending = false;
  speed_index = enemy->definition->post_ai_values[SF_ENEMY_DIRECT_SPEED_INDEX];
  if (speed_index < 0 || speed_index >= 10 || enemy->direction >= 8u) {
    if (enemy->event_number == -1) enemy->event_number = 2;
    sf_enemy_presentation_finish(enemy);
    return;
  }
  animation = &enemy->direct_attack_animations[enemy->direction];
  if (animation->frame_count == 0u ||
      enemy->presentation_previous_frame ==
        (int16_t) animation->frame_count - 1) {
    if (enemy->event_number == -1) enemy->event_number = 2;
    sf_enemy_presentation_finish(enemy);
    return;
  }
  if (enemy->presentation_previous_frame >= 0) ++enemy->presentation_elapsed;
  frame = enemy->presentation_elapsed * sf_enemy_speed_numerator[speed_index] /
    sf_enemy_speed_denominator[speed_index];
  if (animation->frame_count > 0u && frame >= animation->frame_count)
    frame = animation->frame_count - 1;
  enemy->animation_frame = (uint32_t) frame;
  if (animation->part_count > 0u) {
    const SfCafCell *cells = animation->parts[0].cells;
    for (scan = enemy->presentation_previous_frame + 1; scan <= frame;
         ++scan) {
      const int16_t status = cells[scan].status;
      enemy->presentation_audio_markers = (uint8_t) (
        enemy->presentation_audio_markers | sf_enemy_audio_markers(status));
      if ((status & SF_ENEMY_IMPACT_MARKER) != 0)
        enemy->direct_impact_pending = true;
    }
  }
  enemy->presentation_previous_frame = (int16_t) frame;
}
