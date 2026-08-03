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

#include <string.h>

#define SF_SCENARIO_SCRIPT_STACK_LIMIT 16u

typedef struct SfScenarioScriptFrame {
  int32_t sentence;
  uint16_t command;
} SfScenarioScriptFrame;

static int sf_scenario_temporary_index(
    const SfScsScript *script, int32_t id) {
  uint16_t index;
  for (index = 0u; index < script->temporary_flag_count; ++index) {
    if (script->temporary_flags[index].id == id) return (int) index;
  }
  return -1;
}

static bool sf_scenario_state_key(
    int32_t key, int32_t *character_number,
    SfScenarioEntityChannel *channel) {
  if (key >= 300000000 && key < 400000000) {
    *character_number = key - 300000000;
    *channel = SF_SCENARIO_POINTER;
  } else if (key >= 200000000 && key < 300000000) {
    *character_number = key - 200000000;
    *channel = SF_SCENARIO_JUDGEMENT;
  } else if (key >= 100000000 && key < 200000000) {
    *character_number = key - 100000000;
    *channel = SF_SCENARIO_VISIBLE;
  } else {
    return false;
  }
  return true;
}

static int32_t sf_scenario_read(
    const SfScenarioActorScriptState *state, const SfScsScript *script,
    const SfScsOperand *operand, const SfScenarioActorSet *actors) {
  if (operand->type >= 0 && operand->type <= 2) return operand->value;
  if (operand->type == 4) {
    const int index = sf_scenario_temporary_index(script, operand->value);
    return index >= 0 && index < state->temporary_count
      ? state->temporary_values[index] : -1;
  }
  if (operand->type == 5) {
    int32_t character_number;
    SfScenarioEntityChannel channel;
    const SfScenarioActor *actor;
    if (!sf_scenario_state_key(
          operand->value, &character_number, &channel)) return 0;
    actor = sf_scenario_actor_find_const(actors, character_number);
    return actor ? actor->state[channel] : 0;
  }
  if (operand->type == 8) return 0;
  return 0;
}

static bool sf_scenario_write(
    SfScenarioActorScriptState *state, const SfScsScript *script,
    const SfScsOperand *operand, int32_t value,
    SfScenarioActorSet *actors) {
  if (operand->type >= 0 && operand->type <= 2) return true;
  if (operand->type == 4) {
    const int index = sf_scenario_temporary_index(script, operand->value);
    if (index < 0 || index >= state->temporary_count) return false;
    state->temporary_values[index] = value;
    return true;
  }
  if (operand->type == 5) {
    int32_t character_number;
    SfScenarioEntityChannel channel;
    SfScenarioActor *actor;
    if (!sf_scenario_state_key(
          operand->value, &character_number, &channel)) return true;
    actor = sf_scenario_actor_find(actors, character_number);
    if (actor) sf_scenario_actor_set_state(actor, channel, value);
    return true;
  }
  return true;
}

static bool sf_scenario_condition(
    int32_t left, int32_t operation, int32_t right) {
  if (operation == 0) return left == right;
  if (operation == 1) return left != right;
  if (operation == 2) return left > right;
  return operation == 3 && left < right;
}

static bool sf_scenario_set_actor_enabled(
    SfScenarioActorSet *actors, int32_t character_number, int32_t value) {
  SfScenarioActor *actor = sf_scenario_actor_find(actors, character_number);
  if (!actor) return true;
  sf_scenario_actor_set_state(actor, SF_SCENARIO_VISIBLE, value);
  sf_scenario_actor_set_state(actor, SF_SCENARIO_JUDGEMENT, value);
  sf_scenario_actor_set_state(actor, SF_SCENARIO_POINTER, value);
  return true;
}

static bool sf_scenario_actor_script_run_sentence(
    SfScenarioActorScriptState *state, const SfScsScript *script,
    int32_t first_sentence, int32_t companion_type,
    SfScenarioActorSet *actors) {
  SfScenarioScriptFrame stack[SF_SCENARIO_SCRIPT_STACK_LIMIT];
  uint8_t depth = 1u;
  stack[0].sentence = first_sentence;
  stack[0].command = 0u;
  while (depth > 0u) {
    SfScenarioScriptFrame *frame = &stack[depth - 1u];
    const SfScsSentence *sentence = sf_scs_sentence(script, frame->sentence);
    const SfScsCommand *command;
    const SfScsOperand *operand;
    if (!sentence) return false;
    if (frame->command >= sentence->command_count) {
      --depth;
      continue;
    }
    command = &script->commands[sentence->first_command + frame->command++];
    if (command->first_operand + command->operand_count >
        script->operand_count) return false;
    operand = &script->operands[command->first_operand];
    if (command->opcode == 0) {
      if (command->operand_count < 4u) return false;
      if (sf_scenario_condition(
            sf_scenario_read(state, script, &operand[0], actors),
            operand[1].value,
            sf_scenario_read(state, script, &operand[2], actors))) {
        if (depth >= SF_SCENARIO_SCRIPT_STACK_LIMIT ||
            !sf_scs_sentence(script, operand[3].value)) return false;
        stack[depth].sentence = operand[3].value;
        stack[depth].command = 0u;
        ++depth;
      }
    } else if (command->opcode == 1) {
      if (command->operand_count < 2u || !sf_scenario_write(
            state, script, &operand[0],
            sf_scenario_read(state, script, &operand[1], actors), actors))
        return false;
    } else if (command->opcode == 22 || command->opcode == 23) {
      if (command->operand_count < 1u || !sf_scenario_set_actor_enabled(
            actors, sf_scenario_read(state, script, &operand[0], actors),
            command->opcode == 22 ? 1 : 0)) return false;
    } else if (command->opcode == 44) {
      if (command->operand_count < 1u || !sf_scenario_write(
            state, script, &operand[0], companion_type, actors)) return false;
    }
  }
  return true;
}

void sf_scenario_actor_script_init(
    SfScenarioActorScriptState *state, const SfScsScript *script) {
  uint16_t index;
  if (!state) return;
  memset(state, 0, sizeof(*state));
  if (!script) return;
  state->temporary_count = script->temporary_flag_count;
  for (index = 0u; index < state->temporary_count; ++index)
    state->temporary_values[index] = script->temporary_flags[index].initial_value;
}

bool sf_scenario_actor_script_run_periodic(
    SfScenarioActorScriptState *state, const SfScsScript *script,
    int32_t companion_type, SfScenarioActorSet *actors) {
  uint16_t index;
  if (!state || !script || !actors) return false;
  for (index = 0u; index < script->status_count; ++index) {
    const SfScsStatus *status = &script->statuses[index];
    if (status->kind == 5 && !sf_scenario_actor_script_run_sentence(
          state, script, status->sentence, companion_type, actors))
      return false;
  }
  return true;
}
