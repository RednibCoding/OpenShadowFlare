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

#include "interpreter/scenario_script_values.h"

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

int32_t sf_scenario_script_read(
    const SfScenarioActorScriptState *state, const SfScsScript *script,
    const SfScsOperand *operand, const SfScenarioActorSet *actors) {
  if (!state || !script || !operand) return 0;
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
  if (operand->type == 6 || operand->type == 7) {
    const SfScenarioActor *actor = sf_scenario_actor_find_const(
      actors, operand->value);
    if (!actor) return 0;
    return operand->type == 6 ? actor->position.x : actor->position.y;
  }
  if (operand->type == 11 && operand->value >= 0 &&
      operand->value < SF_SCENARIO_SCRIPT_VALUE_LIMIT)
    return state->persistent_values[operand->value];
  if (operand->type == 12 && operand->value >= 0 &&
      operand->value < SF_SCENARIO_SCRIPT_VALUE_LIMIT)
    return state->quest_values[operand->value];
  return 0;
}

bool sf_scenario_script_write(
    SfScenarioActorScriptState *state, const SfScsScript *script,
    const SfScsOperand *operand, int32_t value,
    SfScenarioActorSet *actors) {
  if (!state || !script || !operand) return false;
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
  if (operand->type == 11 && operand->value >= 0 &&
      operand->value < SF_SCENARIO_SCRIPT_VALUE_LIMIT) {
    state->persistent_values[operand->value] = value;
    return true;
  }
  if (operand->type == 12 && operand->value >= 0 &&
      operand->value < SF_SCENARIO_SCRIPT_VALUE_LIMIT) {
    state->quest_values[operand->value] = value;
    return true;
  }
  return true;
}
