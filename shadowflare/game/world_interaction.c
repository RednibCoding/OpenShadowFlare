/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of OpenShadowFlare.
 *
 * OpenShadowFlare is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenShadowFlare is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with OpenShadowFlare. If not, see <https://www.gnu.org/licenses/>.
 */

#include "game/world_interaction.h"

#include "core/coordinates.h"
#include "game/movement.h"
#include "game/player_item.h"
#include "game/world_conversation.h"
#include "game/world_inventory.h"
#include "game/world_script.h"
#include "game/world_transport.h"

#include <limits.h>

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

static SfScenarioObject *sf_world_scenario_object_by_id(
    SfWorldState *world, int32_t object_id) {
  uint8_t index;
  for (index = 0u; index < world->scenario_objects.count; ++index) {
    if (world->scenario_objects.objects[index].id == object_id)
      return &world->scenario_objects.objects[index];
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
  if (definition && item->category == 4u && item->definition_id == 1) {
    if (!sf_player_collect_mine(&world->player)) {
      sf_ground_item_restart_drop(item);
      ++world->ground_items.presentation_revision;
      world->pointer.hovered_ground_item_id = -1;
      return true;
    }
    sf_ground_items_emit_sound(&world->ground_items, 48u);
    world->pointer.hovered_ground_item_id = -1;
    return sf_ground_items_remove(&world->ground_items, id);
  }
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
    sf_player_face_toward(&world->player, actor->position);
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

static void sf_world_refresh_pending_scenario_object(SfWorldState *world) {
  SfScenarioObject *object;
  if (world->pointer.pending_scenario_object_id < 0) return;
  object = sf_world_scenario_object_by_id(
    world, world->pointer.pending_scenario_object_id);
  if (!object || !sf_scenario_object_state(object, SF_SCENARIO_VISIBLE) ||
      !sf_scenario_object_state(object, SF_SCENARIO_POINTER)) {
    world->pointer.pending_scenario_object_id = -1;
    return;
  }
  if (sf_movement_bounds_distance(
        world->player.position, world->player.judgement,
        object->position, object->judgement) <= 0x9f) {
    sf_player_cancel_movement(&world->player);
    world->pointer.pending_scenario_object_id = -1;
    sf_player_face_toward(&world->player, object->position);
    if (world->script) {
      const SfScenarioScriptEnvironment environment =
        sf_world_script_environment(world);
      (void) sf_scenario_actor_script_start_status(
        &world->actor_script_state, world->script, 0,
        sf_scenario_object_character_number(object), &environment);
    }
    world->pointer.hovered_scenario_object_id = -1;
    return;
  }
  sf_player_follow_to(&world->player, object->position);
}

void sf_world_interaction_refresh(SfWorldState *world) {
  if (!world) return;
  sf_world_refresh_pending_actor(world);
  sf_world_refresh_pending_scenario_object(world);
  sf_world_refresh_pending_ground_item(world);
}

void sf_world_interaction_read_input(
    SfWorldState *world, const SfGameInput *input) {
  SfWorldPointerControl *pointer;
  bool message_consumed_input;
  bool inventory_consumed_input;
  if (!world || !input) return;
  pointer = &world->pointer;
  pointer->screen_x = input->pointer_x;
  pointer->screen_y = input->pointer_y;
  pointer->active = input->pointer_active;
  pointer->hovered_actor_id = input->world_pointer_resolved
    ? input->pointed_actor_id : -1;
  pointer->hovered_enemy_id = input->world_pointer_resolved
    ? input->pointed_enemy_id : -1;
  pointer->hovered_scenario_object_id = input->world_pointer_resolved
    ? input->pointed_scenario_object_id : -1;
  pointer->hovered_ground_item_id = input->world_pointer_resolved
    ? input->pointed_ground_item_id : -1;
  if (input->transport_selected) {
    (void) sf_world_transport_activate(
      world, input->transport_destination);
    pointer->hovered_actor_id = -1;
    pointer->hovered_enemy_id = -1;
    pointer->hovered_scenario_object_id = -1;
    pointer->hovered_ground_item_id = -1;
    return;
  }
  inventory_consumed_input = sf_world_inventory_update(world, input);
  message_consumed_input = sf_world_conversation_update(world, input) ||
    inventory_consumed_input;
  if (message_consumed_input)
    pointer->hovered_actor_id = -1;
  if (message_consumed_input)
    pointer->hovered_enemy_id = -1;
  if (message_consumed_input)
    pointer->hovered_scenario_object_id = -1;
  if (message_consumed_input)
    pointer->hovered_ground_item_id = -1;
  if (!message_consumed_input && input->pace_toggle_pressed)
    sf_player_toggle_pace(&world->player);
  if (!message_consumed_input && input->companion_toggle_pressed)
    sf_companion_toggle_activity(&world->companion);
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
    pointer->pending_scenario_object_id = -1;
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
    pointer->pending_scenario_object_id = -1;
    pointer->pending_ground_item_id = -1;
    pointer->interaction_command_active = true;
    pointer->ground_command_active = false;
    pointer->continuous_movement = false;
    pointer->hold_updates = 0u;
  } else if (!message_consumed_input && input->pointer_primary_pressed &&
      pointer->hovered_scenario_object_id >= 0) {
    pointer->pending_actor_id = -1;
    pointer->pending_scenario_object_id =
      pointer->hovered_scenario_object_id;
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
    pointer->pending_scenario_object_id = -1;
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
}
