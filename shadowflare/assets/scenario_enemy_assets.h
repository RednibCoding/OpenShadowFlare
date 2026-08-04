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

#ifndef SHADOWFLARE_ASSETS_SCENARIO_ENEMY_ASSETS_H
#define SHADOWFLARE_ASSETS_SCENARIO_ENEMY_ASSETS_H

#include "core/arena.h"
#include "core/coordinates.h"
#include "data/caf.h"
#include "data/mct.h"
#include "data/njp.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SF_SCENARIO_ENEMY_DIRECTION_COUNT 8u
#define SF_SCENARIO_ENEMY_ANIMATION_COUNT 2u

typedef struct SfScenarioEnemyVisual {
  SfCafSelectedAnimation animations
    [SF_SCENARIO_ENEMY_ANIMATION_COUNT][SF_SCENARIO_ENEMY_DIRECTION_COUNT];
  SfNjpSparseResource artwork;
  SfNjpSparseResource shadows;
  int32_t resource_id;
  uint8_t selected_parts;
} SfScenarioEnemyVisual;

typedef struct SfScenarioEnemyAttackAssets {
  SfCafSelectedAnimation animations[SF_SCENARIO_ENEMY_DIRECTION_COUNT];
  SfNjpSparseResource artwork;
  SfNjpSparseResource shadows;
  size_t memory_bytes;
  int32_t resource_id;
  uint16_t chart;
  bool loaded;
} SfScenarioEnemyAttackAssets;

typedef struct SfScenarioEnemyFrameAssets {
  const SfCafSelectedAnimation *animation;
  const SfNjpSparseResource *artwork;
  const SfNjpSparseResource *shadows;
} SfScenarioEnemyFrameAssets;

typedef struct SfScenarioEnemyAssets {
  SfScenarioEnemyAttackAssets attack;
  SfScenarioEnemyVisual *visuals;
  size_t memory_bytes;
  uint16_t visual_count;
} SfScenarioEnemyAssets;

bool sf_scenario_enemy_assets_load(
  SfScenarioEnemyAssets *assets, const char *data_root,
  const SfMctScenario *scenario, SfWorldPoint focus, SfArena *arena);
const SfScenarioEnemyVisual *sf_scenario_enemy_visual(
  const SfScenarioEnemyAssets *assets, int32_t resource_id);
bool sf_scenario_enemy_attack_assets_load(
  SfScenarioEnemyAttackAssets *attack, const char *data_root,
  const SfMctScenario *scenario, int32_t resource_id, uint16_t chart,
  SfArena *arena);
bool sf_scenario_enemy_frame_assets(
  const SfScenarioEnemyAssets *assets, int32_t resource_id,
  uint16_t chart, uint8_t direction, SfScenarioEnemyFrameAssets *frame);

#endif
