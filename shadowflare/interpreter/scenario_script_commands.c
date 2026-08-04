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

#include "interpreter/scenario_script_commands.h"

#include "game/movement.h"
#include "interpreter/scenario_script_values.h"

static bool sf_scenario_script_condition(
    int32_t left, int32_t operation, int32_t right) {
  if (operation == 0) return left == right;
  if (operation == 1) return left != right;
  if (operation == 2) return left > right;
  return operation == 3 && left < right;
}

static int32_t sf_scenario_script_wrapped_i32(uint32_t value) {
  if (value <= (uint32_t) INT32_MAX) return (int32_t) value;
  return INT32_MIN + (int32_t) (value - UINT32_C(0x80000000));
}

static SfScenarioScriptResult sf_scenario_script_native(
    SfScenarioScriptContext *context, const SfScsCommand *command,
    const SfScsOperand *operands, uint8_t argument_count) {
  int32_t arguments[6];
  uint8_t index;
  if (command->operand_count < argument_count || argument_count > 6u)
    return SF_SCENARIO_SCRIPT_INVALID;
  if (!context->environment->native_command) {
    context->state->unsupported_opcode = command->opcode;
    return SF_SCENARIO_SCRIPT_UNSUPPORTED_COMMAND;
  }
  for (index = 0u; index < argument_count; ++index)
    arguments[index] = sf_scenario_script_read(
      context->state, context->script, &operands[index],
      context->environment->actors);
  return context->environment->native_command(
      context->environment->native_user, command->opcode,
      arguments, argument_count)
    ? SF_SCENARIO_SCRIPT_COMPLETE : SF_SCENARIO_SCRIPT_INVALID;
}

static bool sf_scenario_script_set_actor_enabled(
    SfScenarioActorSet *actors, int32_t character_number, int32_t value) {
  SfScenarioActor *actor = sf_scenario_actor_find(actors, character_number);
  if (!actor) return true;
  sf_scenario_actor_set_state(actor, SF_SCENARIO_VISIBLE, value);
  sf_scenario_actor_set_state(actor, SF_SCENARIO_JUDGEMENT, value);
  sf_scenario_actor_set_state(actor, SF_SCENARIO_POINTER, value);
  return true;
}

static SfScenarioScriptResult sf_scenario_script_show_message(
    SfScenarioScriptContext *context, const SfScsCommand *command,
    const SfScsOperand *operands) {
  SfScenarioActorScriptState *state = context->state;
  SfScenarioActorSet *actors = context->environment->actors;
  int32_t id;
  const SfScsMessage *message;
  int32_t mode;
  int32_t callback;
  if (command->operand_count < 5u) return SF_SCENARIO_SCRIPT_INVALID;
  id = sf_scenario_script_read(
    state, context->script, &operands[0], actors);
  message = sf_scs_message(context->script, id);
  if (!message) return SF_SCENARIO_SCRIPT_INVALID;
  mode = sf_scenario_script_read(
    state, context->script, &operands[2], actors);
  state->initial_selection = sf_scenario_script_read(
    state, context->script, &operands[3], actors);
  state->message_id = id;
  state->message_character_number = state->current_character_number;
  if (state->message_actor_id < 0 &&
      state->current_character_number >= 12000000)
    state->message_actor_id = state->current_character_number - 12000000;
  state->message_active = true;
  state->selected_option = state->initial_selection;
  state->message_selection_pending = state->initial_selection >= 0;
  state->message_result_pending = state->initial_selection < 0;
  state->message_selection_operand = operands[1];
  state->message_result_operand = operands[1];
  state->callback_pending = mode == 0 || mode == 1;
  callback = sf_scenario_script_read(
    state, context->script, &operands[4], actors);
  state->callback_character_number = callback == -1
    ? state->current_character_number : callback;
  return SF_SCENARIO_SCRIPT_COMPLETE;
}

static SfScenarioScriptResult sf_scenario_script_actor_command(
    SfScenarioScriptContext *context, const SfScsCommand *command,
    const SfScsOperand *operands) {
  int32_t character_number = sf_scenario_script_read(
    context->state, context->script, &operands[0],
    context->environment->actors);
  SfScenarioActor *actor = sf_scenario_actor_find(
    context->environment->actors, character_number);
  if (!actor) return SF_SCENARIO_SCRIPT_INVALID;
  if (command->opcode == 18) {
    sf_scenario_actor_begin_interaction(actor);
    context->state->message_actor_id = actor->id;
    return SF_SCENARIO_SCRIPT_COMPLETE;
  }
  if (command->opcode == 19) {
    sf_scenario_actor_release_interaction(actor);
    return SF_SCENARIO_SCRIPT_COMPLETE;
  }
  if (command->operand_count < 2u) return SF_SCENARIO_SCRIPT_INVALID;
  character_number = sf_scenario_script_read(
    context->state, context->script, &operands[1],
    context->environment->actors);
  if (character_number == 0) {
    sf_scenario_actor_face_toward(
      actor, context->environment->player_position);
  } else {
    const SfScenarioActor *target = sf_scenario_actor_find_const(
      context->environment->actors, character_number);
    if (!target) return SF_SCENARIO_SCRIPT_INVALID;
    sf_scenario_actor_face_toward(actor, target->position);
  }
  return SF_SCENARIO_SCRIPT_COMPLETE;
}

static const SfMctObject *sf_scenario_script_object(
    const SfMctScenario *scenario, int32_t character_number) {
  uint8_t index;
  if (!scenario || character_number < 10000000 ||
      character_number >= 11000000) return NULL;
  for (index = 0u; index < scenario->object_count; ++index)
    if (scenario->objects[index].id == character_number - 10000000)
      return &scenario->objects[index];
  return NULL;
}

static bool sf_scenario_script_measure_distance(
    SfScenarioScriptContext *context, int32_t character_number,
    int32_t *distance) {
  const SfScenarioActor *actor = sf_scenario_actor_find_const(
    context->environment->actors, character_number);
  SfWorldPoint position;
  SfObjectBounds bounds;
  if (actor) {
    position = actor->position;
    bounds = actor->judgement;
  } else {
    const SfMctObject *object = sf_scenario_script_object(
      context->environment->scenario, character_number);
    if (!object) return false;
    position = (SfWorldPoint) {object->world_x, object->world_y};
    bounds = (SfObjectBounds) {
      object->judgement_left, object->judgement_top,
      object->judgement_right, object->judgement_bottom};
  }
  *distance = sf_movement_bounds_distance(
    context->environment->player_position,
    context->environment->player_bounds, position, bounds);
  return true;
}

SfScenarioScriptResult sf_scenario_script_execute_command(
    SfScenarioScriptContext *context, const SfScsCommand *command) {
  SfScenarioActorScriptState *state = context->state;
  const SfScsOperand *operands;
  if (command->first_operand + command->operand_count >
      context->script->operand_count) return SF_SCENARIO_SCRIPT_INVALID;
  operands = &context->script->operands[command->first_operand];
  if (command->opcode == 0) {
    if (command->operand_count < 4u) return SF_SCENARIO_SCRIPT_INVALID;
    if (sf_scenario_script_condition(
          sf_scenario_script_read(
            state, context->script, &operands[0],
            context->environment->actors),
          operands[1].value,
          sf_scenario_script_read(
            state, context->script, &operands[2],
            context->environment->actors)) &&
        !sf_scenario_script_push_frame(
          state, context->script, operands[3].value,
          state->current_character_number)) return SF_SCENARIO_SCRIPT_INVALID;
    return SF_SCENARIO_SCRIPT_COMPLETE;
  }
  if (command->opcode == 1) {
    if (command->operand_count < 2u || !sf_scenario_script_write(
          state, context->script, &operands[0],
          sf_scenario_script_read(
            state, context->script, &operands[1],
            context->environment->actors),
          context->environment->actors)) return SF_SCENARIO_SCRIPT_INVALID;
    return SF_SCENARIO_SCRIPT_COMPLETE;
  }
  if (command->opcode == 2)
    return sf_scenario_script_show_message(context, command, operands);
  if (command->opcode == 10)
    return sf_scenario_script_native(context, command, operands, 6u);
  if (command->opcode == 11 || command->opcode == 12) {
    uint32_t left;
    uint32_t right;
    int32_t result;
    if (command->operand_count < 2u) return SF_SCENARIO_SCRIPT_INVALID;
    left = (uint32_t) sf_scenario_script_read(
      state, context->script, &operands[0], context->environment->actors);
    right = (uint32_t) sf_scenario_script_read(
      state, context->script, &operands[1], context->environment->actors);
    result = sf_scenario_script_wrapped_i32(command->opcode == 12
      ? left - right : left + right);
    if (!sf_scenario_script_write(
          state, context->script, &operands[0], result,
          context->environment->actors)) return SF_SCENARIO_SCRIPT_INVALID;
    return SF_SCENARIO_SCRIPT_COMPLETE;
  }
  if (command->opcode == 18 || command->opcode == 19 ||
      command->opcode == 21) {
    if (command->operand_count < 1u) return SF_SCENARIO_SCRIPT_INVALID;
    return sf_scenario_script_actor_command(context, command, operands);
  }
  if (command->opcode == 22 || command->opcode == 23) {
    if (command->operand_count < 1u || !sf_scenario_script_set_actor_enabled(
          context->environment->actors,
          sf_scenario_script_read(
            state, context->script, &operands[0],
            context->environment->actors),
          command->opcode == 22 ? 1 : 0)) return SF_SCENARIO_SCRIPT_INVALID;
    return SF_SCENARIO_SCRIPT_COMPLETE;
  }
  if (command->opcode == 34) {
    int32_t distance;
    if (command->operand_count < 2u ||
        !sf_scenario_script_measure_distance(
          context, sf_scenario_script_read(
            state, context->script, &operands[0],
            context->environment->actors), &distance)) {
      state->unsupported_opcode = command->opcode;
      return SF_SCENARIO_SCRIPT_UNSUPPORTED_COMMAND;
    }
    if (!sf_scenario_script_write(
          state, context->script, &operands[1], distance,
          context->environment->actors)) return SF_SCENARIO_SCRIPT_INVALID;
    return SF_SCENARIO_SCRIPT_COMPLETE;
  }
  if (command->opcode == 38) {
    if (command->operand_count != 1u) return SF_SCENARIO_SCRIPT_INVALID;
    return SF_SCENARIO_SCRIPT_COMPLETE;
  }
  if (command->opcode == 46) {
    if (command->operand_count != 2u) return SF_SCENARIO_SCRIPT_INVALID;
    return SF_SCENARIO_SCRIPT_COMPLETE;
  }
  if (command->opcode == 44) {
    if (command->operand_count < 1u || !sf_scenario_script_write(
          state, context->script, &operands[0],
          context->environment->companion_type,
          context->environment->actors)) return SF_SCENARIO_SCRIPT_INVALID;
    return SF_SCENARIO_SCRIPT_COMPLETE;
  }
  state->unsupported_opcode = command->opcode;
  return SF_SCENARIO_SCRIPT_UNSUPPORTED_COMMAND;
}
