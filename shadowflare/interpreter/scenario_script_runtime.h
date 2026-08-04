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

#ifndef SHADOWFLARE_INTERPRETER_SCENARIO_SCRIPT_RUNTIME_H
#define SHADOWFLARE_INTERPRETER_SCENARIO_SCRIPT_RUNTIME_H

#include "interpreter/scenario_actor_script.h"

typedef struct SfScenarioScriptContext {
  SfScenarioActorScriptState *state;
  const SfScsScript *script;
  const SfScenarioScriptEnvironment *environment;
} SfScenarioScriptContext;

bool sf_scenario_script_push_frame(
  SfScenarioActorScriptState *state, const SfScsScript *script,
  int32_t sentence, int32_t character_number);

#endif
