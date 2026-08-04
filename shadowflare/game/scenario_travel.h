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

#ifndef SHADOWFLARE_GAME_SCENARIO_TRAVEL_H
#define SHADOWFLARE_GAME_SCENARIO_TRAVEL_H

#include <stdbool.h>
#include <stdint.h>

typedef struct SfScenarioTravelRequest {
  int32_t scenario_id;
  int32_t entry_value;
  bool pending;
} SfScenarioTravelRequest;

void sf_scenario_travel_clear(SfScenarioTravelRequest *request);
bool sf_scenario_travel_request(
  SfScenarioTravelRequest *request,
  int32_t scenario_id, int32_t entry_value);

#endif
