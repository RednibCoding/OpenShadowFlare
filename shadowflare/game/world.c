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
#include "game/world_conversation.h"
#include "game/world_inventory.h"
#include "game/world_script.h"

#include <string.h>

void sf_world_state_init(
    SfWorldState *world, int32_t scenario_id, int32_t entry_key,
    uint8_t player_gender) {
  if (!world) return;
  memset(world, 0, sizeof(*world));
  world->scenario_id = scenario_id;
  world->entry_key = entry_key;
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
  world->pointer.hovered_ground_item_id = -1;
  world->pointer.pending_actor_id = -1;
  world->pointer.pending_ground_item_id = -1;
  world->pointer.range = 2u;
  world->pointer.range_enabled = true;
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

void sf_world_state_bind_ground_items(
    SfWorldState *world, const SfItemGroundDefinition *definitions,
    uint8_t definition_count) {
  if (!world) return;
  sf_ground_items_bind_definitions(
    &world->ground_items, definitions, definition_count);
}

bool sf_world_state_bind_scenario(
    SfWorldState *world,
    const SfMctScenario *scenario, const SfScsScript *script) {
  SfScenarioScriptEnvironment environment;
  if (!world || !scenario || !script) return false;
  sf_scenario_actors_init(&world->actors, scenario);
  sf_scenario_actor_script_init(&world->actor_script_state, script);
  world->scenario = scenario;
  world->script = script;
  environment = sf_world_script_environment(world);
  return sf_scenario_actor_script_run_periodic(
    &world->actor_script_state, script, &environment);
}

static SfWorldPoint sf_world_pointer_target(
    const SfWorldState *world, const SfGameInput *input) {
  return sf_screen_to_world((SfScreenPoint) {
    input->pointer_x + world->camera_x + input->world_view_offset_x,
    input->pointer_y + world->camera_y});
}

static SfScenarioActor *sf_world_actor_by_id(
    SfWorldState *world, int32_t actor_id) {
  uint8_t index;
  for (index = 0u; index < world->actors.count; ++index) {
    if (world->actors.actors[index].id == actor_id)
      return &world->actors.actors[index];
  }
  return NULL;
}

static bool sf_world_take_ground_item(
    SfWorldState *world, SfGroundItem *item) {
  const SfItemGroundDefinition *definition;
  SfInventoryItem inventory_item;
  const int32_t id = item ? item->id : -1;
  if (!item) return false;
  definition = sf_ground_items_definition(
    &world->ground_items, item->category, item->definition_id);
  sf_player_cancel_movement(&world->player);
  world->pointer.pending_ground_item_id = -1;
  if (!definition || definition->inventory_width <= 0 ||
      definition->inventory_width > (int32_t) SF_INVENTORY_WIDTH ||
      definition->inventory_height <= 0 ||
      definition->inventory_height > (int32_t) SF_INVENTORY_HEIGHT) {
    sf_ground_item_restart_drop(item);
    ++world->ground_items.presentation_revision;
    world->pointer.hovered_ground_item_id = -1;
    return true;
  }
  inventory_item = (SfInventoryItem) {
    item->definition_id, item->quantity, item->durability,
    item->category, 0u, 0u,
    (uint8_t) definition->inventory_width,
    (uint8_t) definition->inventory_height, item->identified};
  if (!sf_inventory_store_item(
        &world->player.inventory, inventory_item)) {
    sf_ground_item_restart_drop(item);
    ++world->ground_items.presentation_revision;
    world->pointer.hovered_ground_item_id = -1;
    return true;
  }
  sf_ground_items_emit_sound(
    &world->ground_items,
    item->category == 2u ? 93u :
      item->category == 4u && item->definition_id == 0 ? 85u :
      definition->weight < 60 ? 48u : 47u);
  world->pointer.hovered_ground_item_id = -1;
  return sf_ground_items_remove(&world->ground_items, id);
}

static void sf_world_refresh_pending_ground_item(SfWorldState *world) {
  SfGroundItem *item;
  if (world->pointer.pending_ground_item_id < 0) return;
  item = sf_ground_items_find(
    &world->ground_items, world->pointer.pending_ground_item_id);
  if (!item || !item->visible) {
    world->pointer.pending_ground_item_id = -1;
    return;
  }
  if (sf_movement_bounds_distance(
        world->player.position, world->player.judgement,
        item->position, item->judgement) <= 0x9f) {
    (void) sf_world_take_ground_item(world, item);
    return;
  }
  sf_player_follow_to(&world->player, item->position);
}

static void sf_world_refresh_pending_actor(SfWorldState *world) {
  SfScenarioActor *actor;
  if (world->pointer.pending_actor_id < 0) return;
  actor = sf_world_actor_by_id(world, world->pointer.pending_actor_id);
  if (!actor || !sf_scenario_actor_state(actor, SF_SCENARIO_VISIBLE) ||
      !sf_scenario_actor_state(actor, SF_SCENARIO_POINTER)) {
    world->pointer.pending_actor_id = -1;
    return;
  }
  if (sf_movement_bounds_distance(
        world->player.position, world->player.judgement,
        actor->position, actor->judgement) <= 0x9f) {
    sf_player_cancel_movement(&world->player);
    world->pointer.pending_actor_id = -1;
    if (world->script) {
      const SfScenarioScriptEnvironment environment =
        sf_world_script_environment(world);
      (void) sf_scenario_actor_script_start_status(
        &world->actor_script_state, world->script, 0,
        sf_scenario_actor_character_number(actor), &environment);
    }
    if (!world->actor_script_state.message_active)
      sf_scenario_actor_release_interaction(actor);
    world->pointer.hovered_actor_id = -1;
    return;
  }
  sf_player_follow_to(&world->player, actor->position);
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
  bool message_consumed_input;
  bool inventory_consumed_input;
  if (!world || !world->entered || !input) return;
  sf_ground_items_update(&world->ground_items);
  pointer = &world->pointer;
  pointer->screen_x = input->pointer_x;
  pointer->screen_y = input->pointer_y;
  pointer->active = input->pointer_active;
  pointer->hovered_actor_id = input->world_pointer_resolved
    ? input->pointed_actor_id : -1;
  pointer->hovered_ground_item_id = input->world_pointer_resolved
    ? input->pointed_ground_item_id : -1;
  inventory_consumed_input = sf_world_inventory_update(world, input);
  message_consumed_input = sf_world_conversation_update(world, input) ||
    inventory_consumed_input;
  if (message_consumed_input)
    pointer->hovered_actor_id = -1;
  if (message_consumed_input)
    pointer->hovered_ground_item_id = -1;
  if (!message_consumed_input && input->pace_toggle_pressed)
    sf_player_toggle_pace(&world->player);
  if (!message_consumed_input && input->pointer_primary_down &&
      pointer->ground_command_active && !pointer->inventory_pointer_guard) {
    if (input->pointer_primary_pressed) pointer->hold_updates = 1u;
    else if (pointer->hold_updates < UINT8_MAX) ++pointer->hold_updates;
    pointer->continuous_movement = pointer->hold_updates > 9u;
  }
  if (!message_consumed_input && input->pointer_primary_pressed &&
      pointer->hovered_ground_item_id >= 0) {
    SfGroundItem *item = sf_ground_items_find(
      &world->ground_items, pointer->hovered_ground_item_id);
    pointer->pending_actor_id = -1;
    pointer->pending_ground_item_id = pointer->hovered_ground_item_id;
    pointer->interaction_command_active = true;
    pointer->ground_command_active = false;
    pointer->continuous_movement = false;
    pointer->hold_updates = 0u;
    if (item && sf_movement_bounds_distance(
          world->player.position, world->player.judgement,
          item->position, (SfObjectBounds) {0, 0, 0, 0}) <= 0x9f)
      (void) sf_world_take_ground_item(world, item);
  } else if (!message_consumed_input && input->pointer_primary_pressed &&
      pointer->hovered_actor_id >= 0) {
    pointer->pending_actor_id = pointer->hovered_actor_id;
    pointer->pending_ground_item_id = -1;
    pointer->interaction_command_active = true;
    pointer->ground_command_active = false;
    pointer->continuous_movement = false;
    pointer->hold_updates = 0u;
  } else if (!message_consumed_input &&
             (input->pointer_primary_pressed || input->pointer_primary_down) &&
             !input->pointer_over_gameplay_ui &&
             !pointer->inventory_pointer_guard &&
             !pointer->interaction_command_active) {
    const SfWorldPoint target = sf_world_pointer_target(world, input);
    pointer->pending_actor_id = -1;
    pointer->pending_ground_item_id = -1;
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
    pointer->interaction_command_active = false;
    pointer->inventory_pointer_guard = false;
  } else if (input->pointer_primary_pressed && !input->pointer_primary_down) {
    pointer->ground_command_active = false;
    pointer->hold_updates = 0u;
  } else if (pointer->interaction_command_active &&
             !input->pointer_primary_down) {
    pointer->interaction_command_active = false;
  }
  if (!input->pointer_primary_down)
    pointer->inventory_pointer_guard = false;
  sf_world_refresh_pending_actor(world);
  sf_world_refresh_pending_ground_item(world);
  sf_world_build_actor_blockers(world, actor_blocker_indices);
  collision.world = &world->collision;
  collision.blockers = world->movement_blockers;
  collision.blocker_count = world->movement_blocker_count;
  collision.ignored_blocker_id = INT32_MIN;
  sf_player_update_query(&world->player, &collision);
  sf_world_refresh_pending_actor(world);
  sf_world_refresh_pending_ground_item(world);
  sf_world_add_player_blocker(world);
  collision.blocker_count = world->movement_blocker_count;
  sf_world_update_scenario_actors(
    world, actor_blocker_indices, collision);
  if (world->script) {
    const SfScenarioScriptEnvironment environment =
      sf_world_script_environment(world);
    (void) sf_scenario_actor_script_run_periodic(
      &world->actor_script_state, world->script, &environment);
  }
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
