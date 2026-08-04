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

#include "interpreter/scenario_actor_script.h"

#include "interpreter/scenario_script_commands.h"
#include "interpreter/scenario_script_runtime.h"
#include "interpreter/scenario_script_values.h"

#include <string.h>

static const SfScsStatus *sf_scenario_script_status(
    const SfScsScript *script, int32_t kind, int32_t character_number) {
  uint16_t index;
  for (index = 0u; index < script->status_count; ++index) {
    const SfScsStatus *status = &script->statuses[index];
    if (status->kind == kind &&
        status->character_number == character_number) return status;
  }
  return NULL;
}

bool sf_scenario_script_push_frame(
    SfScenarioActorScriptState *state, const SfScsScript *script,
    int32_t sentence, int32_t character_number) {
  SfScenarioScriptFrame *frame;
  if (state->frame_depth >= SF_SCENARIO_SCRIPT_STACK_LIMIT ||
      !sf_scs_sentence(script, sentence)) return false;
  frame = &state->frames[state->frame_depth++];
  frame->sentence = sentence;
  frame->character_number = character_number;
  frame->command = 0u;
  return true;
}

static SfScenarioScriptResult sf_scenario_script_run(
    SfScenarioScriptContext *context) {
  SfScenarioActorScriptState *state = context->state;
  while (state->frame_depth > 0u) {
    SfScenarioScriptFrame *frame = &state->frames[state->frame_depth - 1u];
    const SfScsSentence *sentence = sf_scs_sentence(
      context->script, frame->sentence);
    const SfScsCommand *command;
    SfScenarioScriptResult result;
    if (!sentence) return SF_SCENARIO_SCRIPT_INVALID;
    state->current_character_number = frame->character_number;
    if (frame->command >= sentence->command_count) {
      --state->frame_depth;
      continue;
    }
    command = &context->script->commands[
      sentence->first_command + frame->command++];
    result = sf_scenario_script_execute_command(context, command);
    if (result != SF_SCENARIO_SCRIPT_COMPLETE) return result;
  }
  return state->message_active
    ? SF_SCENARIO_SCRIPT_WAITING_FOR_MESSAGE : SF_SCENARIO_SCRIPT_COMPLETE;
}

static SfScenarioScriptResult sf_scenario_script_enter_status(
    SfScenarioScriptContext *context, int32_t kind,
    int32_t character_number, bool missing_is_complete) {
  const SfScsStatus *status = sf_scenario_script_status(
    context->script, kind, character_number);
  context->state->frame_depth = 0u;
  context->state->unsupported_opcode = -1;
  if (!status) return missing_is_complete
    ? SF_SCENARIO_SCRIPT_COMPLETE : SF_SCENARIO_SCRIPT_INVALID;
  if (!sf_scenario_script_push_frame(
        context->state, context->script,
        status->sentence, character_number)) return SF_SCENARIO_SCRIPT_INVALID;
  return sf_scenario_script_run(context);
}

void sf_scenario_actor_script_init(
    SfScenarioActorScriptState *state, const SfScsScript *script) {
  uint16_t index;
  if (!state) return;
  memset(state, 0, sizeof(*state));
  state->message_id = -1;
  state->message_actor_id = -1;
  state->selected_option = -1;
  state->unsupported_opcode = -1;
  if (!script) return;
  state->temporary_count = script->temporary_flag_count;
  for (index = 0u; index < state->temporary_count; ++index)
    state->temporary_values[index] = script->temporary_flags[index].initial_value;
}

SfScenarioScriptResult sf_scenario_actor_script_start_status(
    SfScenarioActorScriptState *state, const SfScsScript *script,
    int32_t kind, int32_t character_number,
    const SfScenarioScriptEnvironment *environment) {
  SfScenarioScriptContext context;
  if (!state || !script || !environment || !environment->actors)
    return SF_SCENARIO_SCRIPT_INVALID;
  state->message_active = false;
  state->callback_pending = false;
  state->message_result_pending = false;
  state->message_selection_pending = false;
  state->message_id = -1;
  state->message_actor_id = -1;
  state->selected_option = -1;
  context = (SfScenarioScriptContext) {state, script, environment};
  return sf_scenario_script_enter_status(
    &context, kind, character_number, false);
}

SfScenarioScriptResult sf_scenario_actor_script_resume(
    SfScenarioActorScriptState *state, const SfScsScript *script,
    int32_t selection, const SfScenarioScriptEnvironment *environment) {
  SfScenarioScriptContext context;
  int32_t callback;
  if (!state || !script || !environment || !environment->actors)
    return SF_SCENARIO_SCRIPT_INVALID;
  if (!state->message_active) return SF_SCENARIO_SCRIPT_COMPLETE;
  state->message_active = false;
  if (state->message_result_pending && !sf_scenario_script_write(
        state, script, &state->message_result_operand,
        selection, environment->actors)) return SF_SCENARIO_SCRIPT_INVALID;
  if (state->message_selection_pending) {
    const int32_t selected = selection >= 0 ? selection : state->initial_selection;
    if (selected < 0 || !sf_scenario_script_write(
          state, script, &state->message_selection_operand,
          selected, environment->actors)) return SF_SCENARIO_SCRIPT_INVALID;
  }
  state->message_result_pending = false;
  state->message_selection_pending = false;
  context = (SfScenarioScriptContext) {state, script, environment};
  if (!state->callback_pending) return sf_scenario_script_run(&context);
  callback = state->callback_character_number;
  state->callback_pending = false;
  return sf_scenario_script_enter_status(&context, 1, callback, true);
}

bool sf_scenario_actor_script_run_periodic(
    SfScenarioActorScriptState *state, const SfScsScript *script,
    const SfScenarioScriptEnvironment *environment) {
  SfScenarioScriptContext context;
  uint16_t index;
  if (!state || !script || !environment || !environment->actors) return false;
  if (state->message_active) return true;
  context = (SfScenarioScriptContext) {state, script, environment};
  for (index = 0u; index < script->status_count; ++index) {
    const SfScsStatus *status = &script->statuses[index];
    SfScenarioScriptResult result;
    if (status->kind != 5) continue;
    state->frame_depth = 0u;
    if (!sf_scenario_script_push_frame(
          state, script, status->sentence,
          status->character_number)) return false;
    result = sf_scenario_script_run(&context);
    if (result == SF_SCENARIO_SCRIPT_INVALID ||
        result == SF_SCENARIO_SCRIPT_UNSUPPORTED_COMMAND) return false;
  }
  return true;
}

const char *sf_scenario_actor_script_message_text(
    const SfScenarioActorScriptState *state, const SfScsScript *script) {
  const SfScsMessage *message;
  if (!state || !script || !state->message_active) return NULL;
  message = sf_scs_message(script, state->message_id);
  return sf_scs_message_text(script, message);
}

void sf_scenario_actor_script_select_option(
    SfScenarioActorScriptState *state, int32_t option) {
  if (state && state->message_active &&
      state->message_selection_pending && option >= 0)
    state->selected_option = option;
}
