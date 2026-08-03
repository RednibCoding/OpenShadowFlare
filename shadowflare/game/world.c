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

#include "game/world.h"

#include "core/coordinates.h"

#include <string.h>

void sf_world_state_init(
    SfWorldState *world, int32_t scenario_id, int32_t entry_key,
    uint8_t player_gender) {
  if (!world) return;
  memset(world, 0, sizeof(*world));
  world->scenario_id = scenario_id;
  world->entry_key = entry_key;
  sf_player_init(&world->player, player_gender);
}

void sf_world_state_enter(
    SfWorldState *world,
    int32_t player_x, int32_t player_y, uint8_t direction) {
  SfScreenPoint screen;
  if (!world || direction > 7u) return;
  memset(&world->pointer, 0, sizeof(world->pointer));
  sf_player_enter(
    &world->player, (SfWorldPoint) {player_x, player_y}, direction);
  screen = sf_world_to_screen(world->player.position);
  world->camera_x = screen.x - 320;
  world->camera_y = screen.y - 240;
  world->entered = true;
}

void sf_world_state_bind_collision(
    SfWorldState *world,
    const SfGroundMap *ground, const SfObjectMap *objects) {
  if (!world) return;
  world->collision.ground = ground;
  world->collision.objects = objects;
}

bool sf_world_state_bind_scenario(
    SfWorldState *world,
    const SfMctScenario *scenario, const SfScsScript *script) {
  if (!world || !scenario || !script) return false;
  sf_scenario_actors_init(&world->actors, scenario);
  sf_scenario_actor_script_init(&world->actor_script_state, script);
  world->script = script;
  return sf_scenario_actor_script_run_periodic(
    &world->actor_script_state, script,
    world->companion_type, &world->actors);
}

static SfWorldPoint sf_world_pointer_target(
    const SfWorldState *world, const SfGameInput *input) {
  return sf_screen_to_world((SfScreenPoint) {
    input->pointer_x + world->camera_x,
    input->pointer_y + world->camera_y});
}

static void sf_world_build_actor_blockers(
    SfWorldState *world, uint8_t *actor_blocker_indices) {
  uint8_t actor_index;
  world->movement_blocker_count = 0u;
  for (actor_index = 0u; actor_index < world->actors.count; ++actor_index) {
    const SfScenarioActor *actor = &world->actors.actors[actor_index];
    actor_blocker_indices[actor_index] = UINT8_MAX;
    if (!sf_scenario_actor_state(actor, SF_SCENARIO_JUDGEMENT)) continue;
    actor_blocker_indices[actor_index] = world->movement_blocker_count;
    world->movement_blockers[world->movement_blocker_count++] =
      (SfMovementBlocker) {
        actor->position, actor->judgement,
        sf_scenario_actor_character_number(actor)};
  }
}

static void sf_world_add_player_blocker(SfWorldState *world) {
  if (world->movement_blocker_count >= SF_WORLD_MOVEMENT_BLOCKER_LIMIT)
    return;
  world->movement_blockers[world->movement_blocker_count++] =
    (SfMovementBlocker) {
      world->player.position, world->player.judgement, INT32_MIN + 1};
}

static void sf_world_update_scenario_actors(
    SfWorldState *world, const uint8_t *actor_blocker_indices,
    SfCollisionQuery collision) {
  uint8_t actor_index;
  for (actor_index = 0u; actor_index < world->actors.count; ++actor_index) {
    SfScenarioActor *actor = &world->actors.actors[actor_index];
    collision.ignored_blocker_id =
      sf_scenario_actor_character_number(actor);
    sf_scenario_actor_update(actor, &collision);
    if (actor_blocker_indices[actor_index] != UINT8_MAX)
      world->movement_blockers[
        actor_blocker_indices[actor_index]].position = actor->position;
  }
}

void sf_world_state_update(SfWorldState *world, const SfGameInput *input) {
  SfWorldPointerControl *pointer;
  SfCollisionQuery collision;
  SfScreenPoint player_screen;
  uint8_t actor_blocker_indices[SF_MCT_PERSON_LIMIT];
  if (!world || !world->entered || !input) return;
  pointer = &world->pointer;
  if (input->pace_toggle_pressed) sf_player_toggle_pace(&world->player);
  if (input->pointer_primary_down && pointer->ground_command_active) {
    if (input->pointer_primary_pressed) pointer->hold_updates = 1u;
    else if (pointer->hold_updates < UINT8_MAX) ++pointer->hold_updates;
    pointer->continuous_movement = pointer->hold_updates > 9u;
  }
  if (input->pointer_primary_pressed || input->pointer_primary_down) {
    const SfWorldPoint target = sf_world_pointer_target(world, input);
    if (input->pointer_primary_down)
      sf_player_follow_to(&world->player, target);
    else
      sf_player_move_to(&world->player, target);
    if (input->pointer_primary_pressed) {
      pointer->ground_command_active = input->pointer_primary_down;
      pointer->continuous_movement = false;
      pointer->hold_updates = input->pointer_primary_down ? 1u : 0u;
    }
  }
  if (pointer->previous_down && !input->pointer_primary_down) {
    if (pointer->ground_command_active && pointer->continuous_movement)
      sf_player_cancel_movement(&world->player);
    pointer->ground_command_active = false;
    pointer->continuous_movement = false;
    pointer->hold_updates = 0u;
  } else if (input->pointer_primary_pressed && !input->pointer_primary_down) {
    pointer->ground_command_active = false;
    pointer->hold_updates = 0u;
  }
  sf_world_build_actor_blockers(world, actor_blocker_indices);
  collision.world = &world->collision;
  collision.blockers = world->movement_blockers;
  collision.blocker_count = world->movement_blocker_count;
  collision.ignored_blocker_id = INT32_MIN;
  sf_player_update_query(&world->player, &collision);
  sf_world_add_player_blocker(world);
  collision.blocker_count = world->movement_blocker_count;
  sf_world_update_scenario_actors(
    world, actor_blocker_indices, collision);
  if (world->script) (void) sf_scenario_actor_script_run_periodic(
    &world->actor_script_state, world->script,
    world->companion_type, &world->actors);
  player_screen = sf_world_to_screen(world->player.position);
  world->camera_x = player_screen.x - 320;
  world->camera_y = player_screen.y - 240;
  pointer->previous_down = input->pointer_primary_down;
}

void sf_world_render_view(
    const SfWorldState *world, uint16_t interpolation,
    SfWorldRenderView *view) {
  SfScreenPoint current_screen;
  SfScreenPoint render_screen;
  int32_t anchor_x;
  int32_t anchor_y;
  if (!view) return;
  memset(view, 0, sizeof(*view));
  if (!world) return;
  view->player_position = sf_player_render_position(
    &world->player, interpolation);
  current_screen = sf_world_to_screen(world->player.position);
  render_screen = sf_world_to_screen(view->player_position);
  anchor_x = current_screen.x - world->camera_x;
  anchor_y = current_screen.y - world->camera_y;
  view->camera_x = render_screen.x - anchor_x;
  view->camera_y = render_screen.y - anchor_y;
}
