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

#include "game/world_conversation.h"

static void sf_world_release_conversation_actors(SfWorldState *world) {
  uint8_t index;
  for (index = 0u; index < world->actors.count; ++index)
    sf_scenario_actor_release_interaction(&world->actors.actors[index]);
}

static void sf_world_resume_conversation(
    SfWorldState *world, int32_t option) {
  const SfScenarioScriptEnvironment environment = {
    world->scenario, &world->actors, world->player.position,
    world->player.judgement, world->companion_type};
  (void) sf_scenario_actor_script_resume(
    &world->actor_script_state, world->script, option, &environment);
  if (!world->actor_script_state.message_active)
    sf_world_release_conversation_actors(world);
}

static int32_t sf_world_next_conversation_option(
    const SfScenarioActorScriptState *state, const SfGameInput *input) {
  int32_t option = state->selected_option;
  if (!input->conversation_choices_resolved ||
      input->conversation_option_count == 0u) return option;
  if (option < 0 || option >= input->conversation_option_count)
    option = 0;
  if (input->up_pressed)
    option = option == 0
      ? input->conversation_option_count - 1 : option - 1;
  if (input->down_pressed)
    option = option + 1 >= input->conversation_option_count
      ? 0 : option + 1;
  return option;
}

bool sf_world_conversation_update(
    SfWorldState *world, const SfGameInput *input) {
  SfScenarioActorScriptState *state;
  int32_t option;
  if (!world || !input || !world->actor_script_state.message_active)
    return false;
  state = &world->actor_script_state;
  sf_player_cancel_movement(&world->player);
  if (!state->message_selection_pending) {
    if (input->pointer_primary_pressed || input->confirm_pressed)
      sf_world_resume_conversation(world, -1);
    return true;
  }
  option = sf_world_next_conversation_option(state, input);
  if (input->conversation_choices_resolved &&
      input->pointed_conversation_option >= 0)
    option = input->pointed_conversation_option;
  sf_scenario_actor_script_select_option(state, option);
  if (input->pointer_primary_pressed &&
      input->conversation_choices_resolved &&
      input->pointed_conversation_option >= 0)
    sf_world_resume_conversation(world, input->pointed_conversation_option);
  else if (input->confirm_pressed)
    sf_world_resume_conversation(world, option);
  return true;
}
