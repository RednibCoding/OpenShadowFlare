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

#define SF_SCENARIO_SCRIPT_STACK_LIMIT 16u
#define SF_SCENARIO_SCRIPT_VALUE_LIMIT 256

typedef enum SfScenarioScriptResult {
  SF_SCENARIO_SCRIPT_COMPLETE = 0,
  SF_SCENARIO_SCRIPT_WAITING_FOR_MESSAGE,
  SF_SCENARIO_SCRIPT_UNSUPPORTED_COMMAND,
  SF_SCENARIO_SCRIPT_INVALID
} SfScenarioScriptResult;

typedef struct SfScenarioScriptFrame {
  int32_t sentence;
  int32_t character_number;
  uint16_t command;
} SfScenarioScriptFrame;

typedef struct SfScenarioActorScriptState {
  int32_t temporary_values[SF_SCS_FLAG_LIMIT];
  int32_t persistent_values[SF_SCENARIO_SCRIPT_VALUE_LIMIT];
  int32_t quest_values[SF_SCENARIO_SCRIPT_VALUE_LIMIT];
  SfScenarioScriptFrame frames[SF_SCENARIO_SCRIPT_STACK_LIMIT];
  SfScsOperand message_result_operand;
  SfScsOperand message_selection_operand;
  int32_t current_character_number;
  int32_t message_id;
  int32_t message_character_number;
  int32_t message_actor_id;
  int32_t callback_character_number;
  int32_t selected_option;
  int32_t initial_selection;
  int32_t unsupported_opcode;
  uint16_t temporary_count;
  uint8_t frame_depth;
  bool message_active;
  bool callback_pending;
  bool message_result_pending;
  bool message_selection_pending;
} SfScenarioActorScriptState;

typedef bool (*SfScenarioNativeCommand)(
  void *user, int32_t opcode, const int32_t *arguments,
  uint8_t argument_count);

typedef struct SfScenarioScriptEnvironment {
  const SfMctScenario *scenario;
  SfScenarioActorSet *actors;
  SfWorldPoint player_position;
  SfObjectBounds player_bounds;
  int32_t companion_type;
  SfScenarioNativeCommand native_command;
  void *native_user;
} SfScenarioScriptEnvironment;

void sf_scenario_actor_script_init(
  SfScenarioActorScriptState *state, const SfScsScript *script);
bool sf_scenario_actor_script_run_periodic(
  SfScenarioActorScriptState *state, const SfScsScript *script,
  const SfScenarioScriptEnvironment *environment);
SfScenarioScriptResult sf_scenario_actor_script_start_status(
  SfScenarioActorScriptState *state, const SfScsScript *script,
  int32_t kind, int32_t character_number,
  const SfScenarioScriptEnvironment *environment);
SfScenarioScriptResult sf_scenario_actor_script_resume(
  SfScenarioActorScriptState *state, const SfScsScript *script,
  int32_t selection, const SfScenarioScriptEnvironment *environment);
const char *sf_scenario_actor_script_message_text(
  const SfScenarioActorScriptState *state, const SfScsScript *script);
void sf_scenario_actor_script_select_option(
  SfScenarioActorScriptState *state, int32_t option);

#endif
