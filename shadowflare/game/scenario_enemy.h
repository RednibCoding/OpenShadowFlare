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

#ifndef SHADOWFLARE_GAME_SCENARIO_ENEMY_H
#define SHADOWFLARE_GAME_SCENARIO_ENEMY_H

#include "core/bounds.h"
#include "core/coordinates.h"
#include "data/caf.h"
#include "data/mct.h"
#include "data/ai_control.h"
#include "game/scenario_entity.h"
#include "game/route.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct SfScenarioEnemy {
  const SfMctEnemy *definition;
  const SfAiControl *control;
  SfWorldPoint position;
  SfWorldPoint previous_position;
  SfWorldPoint spawn_position;
  SfWorldPoint movement_destination;
  SfObjectBounds judgement;
  SfObjectBounds patrol_bounds;
  SfRouteController route;
  const SfAiAction *selected_action;
  const SfCafSelectedAnimation *direct_attack_animations;
  int32_t state[SF_MCT_ENTITY_STATE_COUNT];
  int32_t current_life;
  int32_t maximum_life;
  int32_t event_number;
  int32_t current_action;
  int32_t action_counter;
  int32_t patrol_counter;
  int32_t movement_counter;
  int32_t movement_speed;
  int32_t movement_duration;
  int32_t presentation_elapsed;
  int16_t presentation_previous_frame;
  uint16_t direct_attack_chart;
  uint32_t animation_frame;
  uint8_t direction;
  uint8_t enabled_parts;
  uint8_t animation_chart;
  uint8_t presentation_action;
  uint8_t presentation_target;
  uint8_t presentation_audio_markers;
  uint8_t movement_target;
  bool direct_impact_pending;
  bool movement_active;
} SfScenarioEnemy;

typedef struct SfScenarioEnemySet {
  SfScenarioEnemy enemies[SF_MCT_ENEMY_LIMIT];
  uint32_t presentation_revision;
  uint16_t count;
} SfScenarioEnemySet;

typedef struct SfEnemyAttackRequest {
  int32_t resource_id;
  int32_t chart;
} SfEnemyAttackRequest;

void sf_scenario_enemies_init(
  SfScenarioEnemySet *enemies, const SfMctScenario *scenario);
bool sf_scenario_enemies_bind_controls(
  SfScenarioEnemySet *enemies, const SfAiControlCatalog *catalog);
bool sf_scenario_enemies_bind_direct_attack(
  SfScenarioEnemySet *enemies, int32_t resource_id, uint16_t chart,
  const SfCafSelectedAnimation *animations);
void sf_scenario_enemies_unbind_direct_attack(SfScenarioEnemySet *enemies);
bool sf_scenario_enemies_attack_resource_active(
  const SfScenarioEnemySet *enemies, int32_t resource_id, uint16_t chart);
int32_t sf_scenario_enemy_character_number(const SfScenarioEnemy *enemy);
SfWorldPoint sf_scenario_enemy_render_position(
  const SfScenarioEnemy *enemy, uint16_t interpolation);
const SfScenarioEnemy *sf_scenario_enemy_at(
  const SfScenarioEnemySet *enemies, uint16_t index);
const SfScenarioEnemy *sf_scenario_enemy_find_const(
  const SfScenarioEnemySet *enemies, int32_t character_number);
bool sf_scenario_enemy_state(
  const SfScenarioEnemy *enemy, SfScenarioEntityChannel channel);

#endif
