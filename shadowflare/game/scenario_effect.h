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

#ifndef SHADOWFLARE_GAME_SCENARIO_EFFECT_H
#define SHADOWFLARE_GAME_SCENARIO_EFFECT_H

#include <stdbool.h>
#include <stdint.h>

#define SF_SCENARIO_PLACED_EFFECT_LIMIT 128u

typedef struct SfScenarioPlacedEffect {
  int32_t effect_number;
  int32_t world_x;
  int32_t world_y;
  int32_t display_height;
  int32_t direction;
  int32_t judgement_right;
  int32_t judgement_bottom;
} SfScenarioPlacedEffect;

typedef struct SfScenarioPlacedEffectSet {
  SfScenarioPlacedEffect effects[SF_SCENARIO_PLACED_EFFECT_LIMIT];
  uint8_t count;
} SfScenarioPlacedEffectSet;

bool sf_scenario_placed_effect_add(
  SfScenarioPlacedEffectSet *effects, const int32_t *arguments,
  uint8_t argument_count);

#endif
