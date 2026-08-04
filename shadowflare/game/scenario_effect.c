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

#include "game/scenario_effect.h"

#include <string.h>

bool sf_scenario_placed_effect_add(
    SfScenarioPlacedEffectSet *effects, const int32_t *arguments,
    uint8_t argument_count) {
  SfScenarioPlacedEffect effect;
  uint8_t index;
  if (!effects || !arguments || argument_count != 7u) return false;
  effect.effect_number = arguments[0];
  effect.world_x = arguments[1];
  effect.world_y = arguments[2];
  effect.display_height = arguments[5];
  effect.direction = arguments[6] < 0 ? 8 : arguments[6];
  effect.judgement_right = arguments[4];
  effect.judgement_bottom = arguments[3];
  for (index = 0u; index < effects->count; ++index)
    if (memcmp(&effects->effects[index], &effect, sizeof(effect)) == 0)
      return true;
  if (effects->count >= SF_SCENARIO_PLACED_EFFECT_LIMIT) return false;
  effects->effects[effects->count++] = effect;
  return true;
}
