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
#include "game/movement.h"
#include "game/world_interaction.h"
#include "game/world_magic.h"
#include "game/world_script.h"
#include "game/world_transport.h"

#include <limits.h>
#include <string.h>

void sf_world_state_init(
    SfWorldState *world, int32_t scenario_id, int32_t entry_key,
    uint8_t player_gender) {
  if (!world) return;
  memset(world, 0, sizeof(*world));
  world->random_state = 1u;
  world->scenario_id = scenario_id;
  world->entry_key = entry_key;
  world->script_transport_service = -1;
  sf_scenario_travel_clear(&world->travel_request);
  sf_ground_items_init(&world->ground_items);
  sf_player_init(&world->player, player_gender);
}

void sf_world_state_enter(
    SfWorldState *world,
    int32_t player_x, int32_t player_y, uint8_t direction) {
  SfScreenPoint screen;
  if (!world || direction > 7u) return;
  memset(&world->pointer, 0, sizeof(world->pointer));
  world->pointer.hovered_actor_id = -1;
  world->pointer.hovered_scenario_object_id = -1;
  world->pointer.hovered_ground_item_id = -1;
  world->pointer.pending_actor_id = -1;
  world->pointer.pending_scenario_object_id = -1;
  world->pointer.pending_ground_item_id = -1;
  world->pointer.range = 2u;
  world->pointer.range_enabled = true;
  sf_gameplay_service_clear(&world->service_request);
  world->script_transport_service = -1;
  sf_scenario_labels_begin(&world->scenario_labels);
  sf_scenario_labels_end(&world->scenario_labels);
  sf_player_enter(
    &world->player, (SfWorldPoint) {player_x, player_y}, direction);
  sf_companion_relocate(
    &world->companion, world->player.position, direction);
  screen = sf_world_to_screen(world->player.position);
  world->camera_x = screen.x - 320;
  world->camera_y = screen.y - 240;
  world->entered = true;
}

void sf_world_state_bind_transports(
    SfWorldState *world, const SfTransportCatalog *transports) {
  if (!world) return;
  world->transports = transports;
}

void sf_world_state_bind_collision(
    SfWorldState *world,
    const SfGroundMap *ground, const SfObjectMap *objects) {
  if (!world) return;
  world->collision.ground = ground;
  world->collision.objects = objects;
}

bool sf_world_state_bind_ground_items(
    SfWorldState *world, const SfItemGroundDefinition *definitions,
    uint8_t definition_count) {
  if (!world) return false;
  sf_ground_items_bind_definitions(
    &world->ground_items, definitions, definition_count);
  return sf_player_initialize_loadout(
    &world->player, definitions, definition_count);
}

bool sf_world_state_bind_scenario(
    SfWorldState *world,
    const SfMctScenario *scenario, const SfScsScript *script) {
  return sf_world_state_bind_scenario_progress(
    world, scenario, script, NULL);
}

static bool sf_world_state_bind_scenario_internal(
    SfWorldState *world,
    const SfMctScenario *scenario, const SfScsScript *script,
    const SfScenarioProgressState *progress, bool changing_scenario) {
  SfScenarioScriptEnvironment environment;
  if (!world || !scenario || !script) return false;
  memset(&world->placed_effects, 0, sizeof(world->placed_effects));
  sf_scenario_objects_init(&world->scenario_objects, scenario);
  sf_scenario_actors_init(&world->actors, scenario);
  if (changing_scenario) {
    sf_scenario_actor_script_change_scenario(
      &world->actor_script_state, script);
  } else {
    sf_scenario_actor_script_init(&world->actor_script_state, script);
    if (progress && !sf_scenario_actor_script_restore_progress(
          &world->actor_script_state, progress)) return false;
  }
  world->scenario = scenario;
  world->script = script;
  world->script_transport_service = -1;
  world->companion_type = world->player.companions.type;
  environment = sf_world_script_environment(world);
  sf_scenario_labels_begin(&world->scenario_labels);
  {
    const bool result = sf_scenario_actor_script_run_periodic(
      &world->actor_script_state, script, &environment);
    sf_scenario_labels_end(&world->scenario_labels);
    return result;
  }
}

bool sf_world_state_bind_scenario_progress(
    SfWorldState *world,
    const SfMctScenario *scenario, const SfScsScript *script,
    const SfScenarioProgressState *progress) {
  return sf_world_state_bind_scenario_internal(
    world, scenario, script, progress, false);
}

bool sf_world_state_change_scenario(
    SfWorldState *world, int32_t scenario_id, int32_t entry_key,
    const SfMctScenario *scenario, const SfScsScript *script) {
  if (!world || scenario_id < 0 || entry_key < 0) return false;
  world->scenario_id = scenario_id;
  world->entry_key = entry_key;
  sf_ground_items_change_scenario(&world->ground_items);
  return sf_world_state_bind_scenario_internal(
    world, scenario, script, NULL, true);
}

bool sf_world_state_bind_companion(
    SfWorldState *world, const SfCompanionProfile *profile) {
  if (!world || !world->entered || !profile ||
      profile->type != world->player.companions.type ||
      profile->level != sf_player_companion_level(
        &world->player.companions)) return false;
  return sf_companion_bind_profile(
    &world->companion, profile, world->player.position,
    world->player.direction,
    world->player.companions.defeated_updates > 0);
}

static void sf_world_build_scenario_object_blockers(SfWorldState *world) {
  uint8_t index;
  world->movement_blocker_count = 0u;
  for (index = 0u; index < world->scenario_objects.count; ++index) {
    const SfScenarioObject *object = &world->scenario_objects.objects[index];
    if (!sf_scenario_object_state(object, SF_SCENARIO_JUDGEMENT)) continue;
    world->movement_blockers[world->movement_blocker_count++] =
      (SfMovementBlocker) {
        object->position, object->judgement,
        sf_scenario_object_character_number(object)};
  }
}

static void sf_world_build_actor_blockers(
    SfWorldState *world, uint8_t *actor_blocker_indices) {
  uint8_t actor_index;
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

static uint8_t sf_world_add_companion_blocker(SfWorldState *world) {
  const uint8_t index = world->movement_blocker_count;
  if (!world->companion.valid || world->companion.current_life <= 0 ||
      index >= SF_WORLD_MOVEMENT_BLOCKER_LIMIT) return UINT8_MAX;
  world->movement_blockers[world->movement_blocker_count++] =
    (SfMovementBlocker) {
      world->companion.position, world->companion.judgement,
      SF_COMPANION_CHARACTER_NUMBER};
  return index;
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
  SfCollisionQuery collision;
  SfScreenPoint player_screen;
  uint8_t actor_blocker_indices[SF_MCT_PERSON_LIMIT];
  uint8_t companion_blocker_index;
  if (!world || !world->entered || !input) return;
  sf_scenario_labels_begin(&world->scenario_labels);
  sf_ground_items_update(&world->ground_items);
  sf_sound_events_reset(&world->sounds);
  if (input->interface_sound != 0u)
    sf_sound_events_push(&world->sounds, input->interface_sound);
  sf_world_magic_update(&world->player, input);
  sf_world_interaction_read_input(world, input);
  sf_world_interaction_refresh(world);
  sf_world_build_scenario_object_blockers(world);
  sf_world_build_actor_blockers(world, actor_blocker_indices);
  collision.world = &world->collision;
  collision.blockers = world->movement_blockers;
  collision.blocker_count = world->movement_blocker_count;
  collision.ignored_blocker_id = INT32_MIN;
  sf_player_update_query(&world->player, &collision);
  sf_world_interaction_refresh(world);
  sf_world_add_player_blocker(world);
  companion_blocker_index = sf_world_add_companion_blocker(world);
  collision.blocker_count = world->movement_blocker_count;
  sf_scenario_objects_update(&world->scenario_objects);
  sf_world_update_scenario_actors(
    world, actor_blocker_indices, collision);
  collision.blocker_count = world->movement_blocker_count;
  collision.ignored_blocker_id = SF_COMPANION_CHARACTER_NUMBER;
  sf_companion_update_follow(
    &world->companion, &world->player, &collision);
  if (companion_blocker_index != UINT8_MAX)
    world->movement_blockers[companion_blocker_index].position =
      world->companion.position;
  if (world->script) {
    const SfScenarioScriptEnvironment environment =
      sf_world_script_environment(world);
    (void) sf_scenario_actor_script_run_periodic(
      &world->actor_script_state, world->script, &environment);
  }
  if (world->script)
    (void) sf_world_script_run_contact_triggers(world);
  (void) sf_world_travel_apply_local(world);
  sf_scenario_labels_end(&world->scenario_labels);
  player_screen = sf_world_to_screen(world->player.position);
  world->camera_x = player_screen.x - 320;
  world->camera_y = player_screen.y - 240;
  world->pointer.previous_down = input->pointer_primary_down;
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
