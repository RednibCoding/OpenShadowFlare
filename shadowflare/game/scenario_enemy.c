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

#include "game/scenario_enemy.h"

#include <limits.h>
#include <string.h>

#define SF_SCENARIO_ENEMY_CHARACTER_BASE 14000000
#define SF_MCT_ENEMY_MAXIMUM_LIFE_INDEX 8u

void sf_scenario_enemies_init(
    SfScenarioEnemySet *enemies, const SfMctScenario *scenario) {
  uint16_t index;
  if (!enemies) return;
  memset(enemies, 0, sizeof(*enemies));
  if (!scenario) return;
  enemies->count = scenario->enemy_count;
  for (index = 0u; index < enemies->count; ++index) {
    const SfMctEnemy *definition = &scenario->enemies[index];
    SfScenarioEnemy *enemy = &enemies->enemies[index];
    uint8_t part;
    enemy->definition = definition;
    enemy->position.x = definition->world_x;
    enemy->position.y = definition->world_y;
    enemy->previous_position = enemy->position;
    enemy->spawn_position = enemy->position;
    enemy->movement_destination = enemy->position;
    enemy->judgement.left = definition->judgement_left;
    enemy->judgement.top = definition->judgement_top;
    enemy->judgement.right = definition->judgement_right;
    enemy->judgement.bottom = definition->judgement_bottom;
    enemy->patrol_bounds.left = definition->pre_ai_values[1];
    enemy->patrol_bounds.top = definition->pre_ai_values[2];
    enemy->patrol_bounds.right = definition->pre_ai_values[3];
    enemy->patrol_bounds.bottom = definition->pre_ai_values[4];
    memcpy(enemy->state, definition->initial_state, sizeof(enemy->state));
    enemy->maximum_life = definition->pre_ai_values[
      SF_MCT_ENEMY_MAXIMUM_LIFE_INDEX];
    enemy->current_life = enemy->maximum_life;
    enemy->direction = (uint8_t) definition->direction;
    enemy->event_number = 0;
    enemy->current_action = -1;
    enemy->presentation_action = 7u;
    enemy->presentation_target = UINT8_MAX;
    enemy->presentation_previous_frame = -1;
    enemy->direct_attack_chart = UINT16_MAX;
    sf_route_reset(&enemy->route);
    for (part = 0u; part < SF_MCT_PERSON_PART_LIMIT; ++part) {
      if (!definition->custom_parts ||
          definition->part_visibility[part] != 0u)
        enemy->enabled_parts = (uint8_t) (
          enemy->enabled_parts | (uint8_t) (1u << part));
    }
  }
}

bool sf_scenario_enemies_bind_direct_attack(
    SfScenarioEnemySet *enemies, int32_t resource_id, uint16_t chart,
    const SfCafSelectedAnimation *animations) {
  uint16_t index;
  bool matched = false;
  if (!enemies || resource_id < 0 || !animations) return false;
  for (index = 0u; index < enemies->count; ++index) {
    SfScenarioEnemy *enemy = &enemies->enemies[index];
    uint8_t variant;
    if (!enemy->definition || enemy->definition->resource_id != resource_id)
      continue;
    for (variant = 0u; variant < 3u; ++variant) {
      if (enemy->definition->post_ai_values[41u + variant] + 4 == chart)
        break;
    }
    if (variant == 3u) continue;
    enemy->direct_attack_animations = animations;
    enemy->direct_attack_chart = chart;
    matched = true;
  }
  return matched;
}

void sf_scenario_enemies_unbind_direct_attack(SfScenarioEnemySet *enemies) {
  uint16_t index;
  if (!enemies) return;
  for (index = 0u; index < enemies->count; ++index) {
    enemies->enemies[index].direct_attack_animations = NULL;
    enemies->enemies[index].direct_attack_chart = UINT16_MAX;
  }
}

bool sf_scenario_enemies_attack_resource_active(
    const SfScenarioEnemySet *enemies, int32_t resource_id, uint16_t chart) {
  uint16_t index;
  if (!enemies || resource_id < 0) return false;
  for (index = 0u; index < enemies->count; ++index) {
    const SfScenarioEnemy *enemy = &enemies->enemies[index];
    const uint8_t action = enemy->presentation_action;
    if (enemy->definition && action >= 1u && action <= 3u &&
        enemy->definition->resource_id == resource_id &&
        enemy->definition->post_ai_values[40u + action] + 4 == chart)
      return true;
  }
  return false;
}

bool sf_scenario_enemies_bind_controls(
    SfScenarioEnemySet *enemies, const SfAiControlCatalog *catalog) {
  uint16_t index;
  if (!enemies || !catalog) return false;
  for (index = 0u; index < enemies->count; ++index) {
    SfScenarioEnemy *enemy = &enemies->enemies[index];
    enemy->control = enemy->definition
      ? sf_ai_control_find(catalog, enemy->definition->ai_control_name) : NULL;
    if (!enemy->control) return false;
  }
  return true;
}

int32_t sf_scenario_enemy_character_number(const SfScenarioEnemy *enemy) {
  return enemy && enemy->definition
    ? SF_SCENARIO_ENEMY_CHARACTER_BASE + enemy->definition->id : INT32_MIN;
}

SfWorldPoint sf_scenario_enemy_render_position(
    const SfScenarioEnemy *enemy, uint16_t interpolation) {
  if (!enemy) return (SfWorldPoint) {0, 0};
  return sf_world_point_interpolate(
    enemy->previous_position, enemy->position, interpolation);
}

const SfScenarioEnemy *sf_scenario_enemy_at(
    const SfScenarioEnemySet *enemies, uint16_t index) {
  return enemies && index < enemies->count ? &enemies->enemies[index] : NULL;
}

const SfScenarioEnemy *sf_scenario_enemy_find_const(
    const SfScenarioEnemySet *enemies, int32_t character_number) {
  uint16_t index;
  if (!enemies) return NULL;
  for (index = 0u; index < enemies->count; ++index) {
    if (sf_scenario_enemy_character_number(&enemies->enemies[index]) ==
        character_number) return &enemies->enemies[index];
  }
  return NULL;
}

bool sf_scenario_enemy_state(
    const SfScenarioEnemy *enemy, SfScenarioEntityChannel channel) {
  return enemy && channel < SF_MCT_ENTITY_STATE_COUNT &&
    enemy->state[channel] != 0;
}
