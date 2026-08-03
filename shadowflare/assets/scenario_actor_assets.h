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

#ifndef SHADOWFLARE_ASSETS_SCENARIO_ACTOR_ASSETS_H
#define SHADOWFLARE_ASSETS_SCENARIO_ACTOR_ASSETS_H

#include "core/arena.h"
#include "data/caf.h"
#include "data/mct.h"
#include "data/njp.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SF_SCENARIO_ACTOR_DIRECTION_COUNT 8u

typedef struct SfScenarioActorVisual {
  SfCafSelectedAnimation animations[SF_SCENARIO_ACTOR_DIRECTION_COUNT];
  SfNjpSparseResource artwork;
  SfNjpSparseResource shadows;
  int32_t resource_id;
  uint8_t selected_parts;
} SfScenarioActorVisual;

typedef struct SfScenarioActorAssets {
  SfScenarioActorVisual *visuals;
  size_t memory_bytes;
  uint8_t visual_count;
} SfScenarioActorAssets;

bool sf_scenario_actor_assets_load(
  SfScenarioActorAssets *assets, const char *data_root,
  const SfMctScenario *scenario, SfArena *arena);
const SfScenarioActorVisual *sf_scenario_actor_visual(
  const SfScenarioActorAssets *assets, int32_t resource_id);

#endif
