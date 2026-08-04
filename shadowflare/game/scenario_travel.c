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

#include "game/scenario_travel.h"

#include <limits.h>

void sf_scenario_travel_clear(SfScenarioTravelRequest *request) {
  if (!request) return;
  request->scenario_id = 0;
  request->entry_value = 0;
  request->pending = false;
}

bool sf_scenario_travel_request(
    SfScenarioTravelRequest *request,
    int32_t scenario_id, int32_t entry_value) {
  if (!request || request->pending || scenario_id < 0 || entry_value < 0 ||
      entry_value > INT32_MAX / 4) return false;
  request->scenario_id = scenario_id;
  request->entry_value = entry_value;
  request->pending = true;
  return true;
}
