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

#include "game/world_inventory.h"

#include "core/coordinates.h"
#include "game/movement.h"
#include "game/player_item.h"

static uint16_t sf_world_inventory_move_sound(
    const SfItemGroundDefinition *definition) {
  if (definition->category == 2u) return 93u;
  if (definition->category == 4u && definition->definition_id == 0)
    return 85u;
  return definition->weight < 60 ? 48u : 47u;
}

static void sf_world_inventory_cancel_world_command(SfWorldState *world) {
  sf_player_cancel_movement(&world->player);
  world->pointer.pending_actor_id = -1;
  world->pointer.pending_ground_item_id = -1;
  world->pointer.hovered_actor_id = -1;
  world->pointer.hovered_ground_item_id = -1;
  world->pointer.interaction_command_active = false;
  world->pointer.ground_command_active = false;
  world->pointer.continuous_movement = false;
  world->pointer.hold_updates = 0u;
}

static SfWorldPoint sf_world_inventory_drop_position(
    const SfWorldState *world, const SfGameInput *input) {
  const SfWorldPoint pointer = sf_screen_to_world((SfScreenPoint) {
    input->pointer_x + world->camera_x + input->world_view_offset_x,
    input->pointer_y + world->camera_y});
  SfWorldPoint result = world->player.position;
  const uint8_t direction = sf_movement_direction(result, pointer);
  const int32_t distance = 200;
  if (direction == 0u || direction == 1u || direction == 2u)
    result.x += distance;
  if (direction == 4u || direction == 5u || direction == 6u)
    result.x -= distance;
  if (direction == 0u || direction == 6u || direction == 7u)
    result.y += distance;
  if (direction == 2u || direction == 3u || direction == 4u)
    result.y -= distance;
  return result;
}

bool sf_world_inventory_update(
    SfWorldState *world, const SfGameInput *input) {
  SfInventoryTransferState *transfer;
  SfInventoryItem moved_item;
  const SfItemGroundDefinition *definition;
  uint16_t sound = 0u;
  bool equipment_changed = false;
  bool changed = false;
  if (!world || !input || input->inventory_action == SF_INVENTORY_ACTION_NONE)
    return false;
  transfer = &world->player.inventory_transfer;
  moved_item = transfer->held_item;
  world->pointer.inventory_pointer_guard = input->pointer_primary_down;
  if (input->inventory_action == SF_INVENTORY_ACTION_TAKE &&
      !transfer->holding_item && input->inventory_item_index >= 0 &&
      sf_inventory_take(
        &world->player.inventory, (uint8_t) input->inventory_item_index,
        &transfer->held_item)) {
    transfer->holding_item = true;
    moved_item = transfer->held_item;
    changed = true;
  } else if (input->inventory_action ==
               SF_INVENTORY_ACTION_TAKE_EQUIPMENT &&
             !transfer->holding_item && input->equipment_slot >= 0 &&
             input->equipment_slot < SF_EQUIPMENT_SLOT_COUNT &&
             sf_equipment_take(
               &world->player.equipment,
               (SfEquipmentSlot) input->equipment_slot,
               &transfer->held_item)) {
    transfer->holding_item = true;
    moved_item = transfer->held_item;
    equipment_changed = true;
    changed = true;
  } else if (input->inventory_action == SF_INVENTORY_ACTION_PLACE &&
             transfer->holding_item) {
    const SfInventoryPlacement placement = sf_inventory_place(
      &world->player.inventory, transfer->held_item,
      input->inventory_grid_x, input->inventory_grid_y);
    if (placement.accepted) {
      transfer->held_item = placement.held_item;
      transfer->holding_item = placement.holding_item;
      changed = true;
    }
  } else if (input->inventory_action ==
               SF_INVENTORY_ACTION_PLACE_EQUIPMENT &&
             transfer->holding_item && input->equipment_slot >= 0 &&
             input->equipment_slot < SF_EQUIPMENT_SLOT_COUNT) {
    definition = sf_ground_items_definition(
      &world->ground_items, transfer->held_item.category,
      transfer->held_item.definition_id);
    if (definition) {
      const SfEquipmentPlacement placement = sf_equipment_place(
        &world->player.equipment,
        (SfEquipmentSlot) input->equipment_slot,
        transfer->held_item, definition, world->player.level);
      if (placement.accepted) {
        transfer->held_item = placement.held_item;
        transfer->holding_item = placement.holding_item;
        sound = definition->category == 2u ? 93u : 49u;
        equipment_changed = true;
        changed = true;
      }
    }
  } else if (input->inventory_action == SF_INVENTORY_ACTION_TAKE_BELT &&
             !transfer->holding_item && input->belt_grid_x >= 0 &&
             input->belt_grid_y >= 0 && sf_belt_take_at(
               &world->player.belt, (uint8_t) input->belt_grid_x,
               (uint8_t) input->belt_grid_y, &transfer->held_item)) {
    transfer->holding_item = true;
    moved_item = transfer->held_item;
    changed = true;
  } else if (input->inventory_action == SF_INVENTORY_ACTION_PLACE_BELT &&
             transfer->holding_item && input->belt_grid_x >= 0 &&
             input->belt_grid_y >= 0) {
    definition = sf_ground_items_definition(
      &world->ground_items, transfer->held_item.category,
      transfer->held_item.definition_id);
    if (definition) {
      const SfInventoryPlacement placement = sf_belt_place(
        &world->player.belt, transfer->held_item,
        input->belt_grid_x, input->belt_grid_y, definition);
      if (placement.accepted) {
        transfer->held_item = placement.held_item;
        transfer->holding_item = placement.holding_item;
        changed = true;
      }
    }
  } else if (input->inventory_action == SF_INVENTORY_ACTION_USE_BACKPACK &&
             !transfer->holding_item && input->inventory_item_index >= 0 &&
             input->inventory_item_index < world->player.inventory.count) {
    const uint8_t index = (uint8_t) input->inventory_item_index;
    moved_item = world->player.inventory.items[index];
    definition = sf_ground_items_definition(
      &world->ground_items, moved_item.category, moved_item.definition_id);
    if (definition && sf_player_use_medicine(
          &world->player, definition) && sf_inventory_take(
          &world->player.inventory, index, &moved_item)) {
      sound = 16u;
      changed = true;
    }
  } else if (input->inventory_action == SF_INVENTORY_ACTION_USE_BELT &&
             !transfer->holding_item && input->belt_grid_x >= 0 &&
             input->belt_grid_y >= 0) {
    const SfInventoryItem *item = sf_belt_item_at(
      &world->player.belt, (uint8_t) input->belt_grid_x,
      (uint8_t) input->belt_grid_y);
    if (item) {
      moved_item = *item;
      definition = sf_ground_items_definition(
        &world->ground_items, item->category, item->definition_id);
      if (definition && sf_player_use_medicine(
            &world->player, definition) && sf_belt_take_at(
            &world->player.belt, (uint8_t) input->belt_grid_x,
            (uint8_t) input->belt_grid_y, &moved_item)) {
        sound = 16u;
        changed = true;
      }
    }
  } else if (input->inventory_action == SF_INVENTORY_ACTION_DROP_WORLD &&
             transfer->holding_item && sf_ground_items_create_instance(
               &world->ground_items,
               transfer->held_item.category,
               transfer->held_item.definition_id,
               transfer->held_item.quantity,
               transfer->held_item.durability,
               transfer->held_item.identified,
               sf_world_inventory_drop_position(world, input))) {
    transfer->holding_item = false;
    sf_world_inventory_cancel_world_command(world);
    changed = true;
  }
  if (!changed) return true;
  if (equipment_changed) sf_player_refresh_visible_items(&world->player);
  definition = sf_ground_items_definition(
    &world->ground_items, moved_item.category, moved_item.definition_id);
  if (definition)
    sf_ground_items_emit_sound(
      &world->ground_items,
      sound != 0u ? sound : sf_world_inventory_move_sound(definition));
  return true;
}
