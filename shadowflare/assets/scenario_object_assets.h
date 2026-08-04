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

#ifndef SHADOWFLARE_ASSETS_SCENARIO_OBJECT_ASSETS_H
#define SHADOWFLARE_ASSETS_SCENARIO_OBJECT_ASSETS_H

#include "core/arena.h"
#include "data/caf.h"
#include "data/mct.h"
#include "data/njp.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct SfScenarioObjectAnimationKey {
  uint16_t chart;
  uint8_t direction;
} SfScenarioObjectAnimationKey;

typedef struct SfScenarioObjectVisual {
  SfNjpDecodedResource static_artwork;
  SfNjpDecodedResource static_shadows;
  SfNjpSparseResource animation_artwork;
  SfCafSelectedAnimation *animations;
  SfScenarioObjectAnimationKey *animation_keys;
  int32_t resource_id;
  uint8_t animation_count;
} SfScenarioObjectVisual;

typedef struct SfScenarioObjectAssets {
  SfScenarioObjectVisual *visuals;
  size_t memory_bytes;
  uint8_t visual_count;
} SfScenarioObjectAssets;

bool sf_scenario_object_assets_load(
  SfScenarioObjectAssets *assets, const char *data_root,
  const SfMctScenario *scenario, SfArena *arena);
const SfScenarioObjectVisual *sf_scenario_object_visual(
  const SfScenarioObjectAssets *assets, int32_t resource_id);
const SfCafSelectedAnimation *sf_scenario_object_animation(
  const SfScenarioObjectVisual *visual, int32_t chart, uint8_t direction);

#endif
