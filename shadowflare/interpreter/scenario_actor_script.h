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

#ifndef SHADOWFLARE_INTERPRETER_SCENARIO_ACTOR_SCRIPT_H
#define SHADOWFLARE_INTERPRETER_SCENARIO_ACTOR_SCRIPT_H

#include "data/scs.h"
#include "game/scenario_actor.h"

#include <stdbool.h>
#include <stdint.h>

/* The current interpreter slice evaluates periodic actor visibility rows.
 * Other SCS commands stay decoded but gain behavior only with their systems. */
typedef struct SfScenarioActorScriptState {
  int32_t temporary_values[SF_SCS_FLAG_LIMIT];
  uint16_t temporary_count;
} SfScenarioActorScriptState;

void sf_scenario_actor_script_init(
  SfScenarioActorScriptState *state, const SfScsScript *script);
bool sf_scenario_actor_script_run_periodic(
  SfScenarioActorScriptState *state, const SfScsScript *script,
  int32_t companion_type, SfScenarioActorSet *actors);

#endif
