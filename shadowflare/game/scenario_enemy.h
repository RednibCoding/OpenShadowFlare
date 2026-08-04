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
#include "data/mct.h"
#include "data/ai_control.h"
#include "game/scenario_entity.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct SfScenarioEnemy {
  const SfMctEnemy *definition;
  const SfAiControl *control;
  SfWorldPoint position;
  SfWorldPoint previous_position;
  SfObjectBounds judgement;
  int32_t state[SF_MCT_ENTITY_STATE_COUNT];
  int32_t current_life;
  int32_t maximum_life;
  uint32_t animation_frame;
  uint8_t direction;
  uint8_t enabled_parts;
} SfScenarioEnemy;

typedef struct SfScenarioEnemySet {
  SfScenarioEnemy enemies[SF_MCT_ENEMY_LIMIT];
  uint16_t count;
} SfScenarioEnemySet;

void sf_scenario_enemies_init(
  SfScenarioEnemySet *enemies, const SfMctScenario *scenario);
bool sf_scenario_enemies_bind_controls(
  SfScenarioEnemySet *enemies, const SfAiControlCatalog *catalog);
void sf_scenario_enemy_update(SfScenarioEnemy *enemy);
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
