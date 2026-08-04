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

#include "ui/gameplay_inventory_input.h"

#include "ui/gameplay_belt.h"
#include "ui/gameplay_equipment_layout.h"
#include "ui/gameplay_hud_input.h"

static bool sf_inventory_pointer_inside(
    const SfGameInput *input, int left, int top, int right, int bottom) {
  return input->pointer_active && input->pointer_x >= left &&
    input->pointer_x < right && input->pointer_y >= top &&
    input->pointer_y < bottom;
}

bool sf_gameplay_inventory_input_resolve(
    SfGameplayInventoryUi *inventory, const SfPlayerState *player,
    bool conversation_active, SfGameInput *input) {
  bool changed = false;
  bool toggle;
  bool close_hovered;
  bool holding;
  bool pointer_moved = false;
  int8_t hovered = -1;
  int8_t hovered_special = -1;
  SfEquipmentSlot equipment_slot = SF_EQUIPMENT_SLOT_COUNT;
  SfEquipmentSlot information_slot = SF_EQUIPMENT_SLOT_COUNT;
  uint8_t belt_x = 0u;
  uint8_t belt_y = 0u;
  bool belt_pocket;
  if (!inventory || !player || !input) return false;
  sf_gameplay_hud_input_resolve(input);
  if (input->pointer_active) {
    pointer_moved = inventory->pointer_x != input->pointer_x ||
      inventory->pointer_y != input->pointer_y;
    inventory->pointer_x = input->pointer_x;
    inventory->pointer_y = input->pointer_y;
  }
  holding = player->inventory_transfer.holding_item;
  belt_pocket = input->pointer_active && sf_gameplay_belt_pocket_at(
    input->pointer_x, input->pointer_y, &belt_x, &belt_y);
  toggle = input->inventory_pressed ||
    sf_gameplay_hud_button_at_pointer(input) ==
      SF_GAMEPLAY_HUD_BUTTON_INVENTORY;
  if (toggle && (!conversation_active || inventory->open)) {
    inventory->open = !inventory->open;
    changed = true;
  }
  if (input->special_items_pressed &&
      (!conversation_active || inventory->special_open)) {
    inventory->special_open = !inventory->special_open;
    inventory->hovered_special_item_index = -1;
    inventory->item_hover_updates = 0u;
    changed = true;
  }
  close_hovered = inventory->open && sf_inventory_pointer_inside(
    input, 375, 393, 443, 404);
  if (inventory->close_hovered != close_hovered) {
    inventory->close_hovered = close_hovered;
    changed = true;
  }
  if (inventory->open && !holding && sf_inventory_pointer_inside(
        input, SF_GAMEPLAY_INVENTORY_BACKPACK_LEFT,
        SF_GAMEPLAY_INVENTORY_BACKPACK_TOP,
        SF_GAMEPLAY_INVENTORY_BACKPACK_LEFT +
          SF_INVENTORY_WIDTH * SF_GAMEPLAY_INVENTORY_CELL_SIZE,
        SF_GAMEPLAY_INVENTORY_BACKPACK_TOP +
          SF_INVENTORY_HEIGHT * SF_GAMEPLAY_INVENTORY_CELL_SIZE)) {
    hovered = sf_inventory_item_at(
      &player->inventory,
      (uint8_t) ((input->pointer_x -
        SF_GAMEPLAY_INVENTORY_BACKPACK_LEFT) /
        SF_GAMEPLAY_INVENTORY_CELL_SIZE),
      (uint8_t) ((input->pointer_y -
        SF_GAMEPLAY_INVENTORY_BACKPACK_TOP) /
        SF_GAMEPLAY_INVENTORY_CELL_SIZE));
  }
  if (inventory->special_open && !holding &&
      (!inventory->open || input->pointer_x <
        SF_GAMEPLAY_INVENTORY_PANEL_LEFT) &&
      sf_inventory_pointer_inside(
        input, SF_GAMEPLAY_SPECIAL_LEFT, SF_GAMEPLAY_SPECIAL_TOP,
        SF_GAMEPLAY_SPECIAL_LEFT +
          SF_SPECIAL_ITEM_WIDTH * SF_GAMEPLAY_INVENTORY_CELL_SIZE,
        SF_GAMEPLAY_SPECIAL_TOP +
          SF_SPECIAL_ITEM_HEIGHT * SF_GAMEPLAY_INVENTORY_CELL_SIZE)) {
    hovered_special = sf_special_items_item_at(
      &player->special_items,
      (uint8_t) ((input->pointer_x - SF_GAMEPLAY_SPECIAL_LEFT) /
        SF_GAMEPLAY_INVENTORY_CELL_SIZE),
      (uint8_t) ((input->pointer_y - SF_GAMEPLAY_SPECIAL_TOP) /
        SF_GAMEPLAY_INVENTORY_CELL_SIZE));
  }
  if (inventory->open && input->pointer_active)
    equipment_slot = sf_gameplay_equipment_slot_at(
      input->pointer_x, input->pointer_y);
  if (!holding && equipment_slot < SF_EQUIPMENT_VISIBLE_SLOT_COUNT &&
      sf_equipment_item(&player->equipment, equipment_slot))
    information_slot = equipment_slot;
  if (inventory->hovered_item_index == hovered &&
      inventory->hovered_special_item_index == hovered_special &&
      inventory->hovered_equipment_slot ==
        (information_slot < SF_EQUIPMENT_VISIBLE_SLOT_COUNT
          ? (int8_t) information_slot : -1)) {
    if (hovered >= 0 || hovered_special >= 0 ||
        information_slot < SF_EQUIPMENT_VISIBLE_SLOT_COUNT) {
      if (inventory->item_hover_updates < 3u) {
        ++inventory->item_hover_updates;
        if (inventory->item_hover_updates == 3u) changed = true;
      } else if (pointer_moved) {
        changed = true;
      }
    } else {
      inventory->item_hover_updates = 0u;
    }
  } else {
    inventory->item_hover_updates =
      hovered >= 0 || hovered_special >= 0 ||
        information_slot < SF_EQUIPMENT_VISIBLE_SLOT_COUNT
        ? 1u : 0u;
    inventory->hovered_item_index = hovered;
    inventory->hovered_special_item_index = hovered_special;
    inventory->hovered_equipment_slot =
      information_slot < SF_EQUIPMENT_VISIBLE_SLOT_COUNT
        ? (int8_t) information_slot : -1;
    changed = true;
  }
  if ((inventory->open || inventory->special_open) &&
      input->cancel_pressed) {
    inventory->open = false;
    inventory->special_open = false;
    inventory->close_hovered = false;
    inventory->hovered_special_item_index = -1;
    input->cancel_pressed = false;
    changed = true;
  } else if (inventory->open && inventory->close_hovered &&
             input->pointer_primary_pressed) {
    inventory->open = false;
    inventory->close_hovered = false;
    input->pointer_over_gameplay_ui = true;
    changed = true;
  } else if (input->pointer_primary_pressed && belt_pocket) {
    if (holding) {
      input->inventory_action = SF_INVENTORY_ACTION_PLACE_BELT;
      input->belt_grid_x = (int8_t) belt_x;
      input->belt_grid_y = (int8_t) belt_y;
    } else if (sf_belt_item_at(&player->belt, belt_x, belt_y)) {
      input->inventory_action = SF_INVENTORY_ACTION_TAKE_BELT;
      input->belt_grid_x = (int8_t) belt_x;
      input->belt_grid_y = (int8_t) belt_y;
    }
    input->pointer_over_gameplay_ui = true;
    changed = true;
  } else if (inventory->special_open && input->pointer_primary_pressed &&
             sf_inventory_pointer_inside(
               input, SF_GAMEPLAY_SPECIAL_LEFT, SF_GAMEPLAY_SPECIAL_TOP,
               SF_GAMEPLAY_SPECIAL_LEFT +
                 SF_SPECIAL_ITEM_WIDTH * SF_GAMEPLAY_INVENTORY_CELL_SIZE,
               SF_GAMEPLAY_SPECIAL_TOP +
                 SF_SPECIAL_ITEM_HEIGHT * SF_GAMEPLAY_INVENTORY_CELL_SIZE)) {
    if (holding) {
      const SfInventoryItem *item = &player->inventory_transfer.held_item;
      input->inventory_action = SF_INVENTORY_ACTION_PLACE_SPECIAL;
      input->special_grid_x = (int8_t) (
        (input->pointer_x - item->width * SF_GAMEPLAY_INVENTORY_CELL_SIZE / 2 -
         (SF_GAMEPLAY_SPECIAL_LEFT -
          SF_GAMEPLAY_INVENTORY_CELL_SIZE / 2)) /
        SF_GAMEPLAY_INVENTORY_CELL_SIZE);
      input->special_grid_y = (int8_t) (
        (input->pointer_y - item->height * SF_GAMEPLAY_INVENTORY_CELL_SIZE / 2 -
         (SF_GAMEPLAY_SPECIAL_TOP -
          SF_GAMEPLAY_INVENTORY_CELL_SIZE / 2)) /
        SF_GAMEPLAY_INVENTORY_CELL_SIZE);
    } else if (inventory->hovered_special_item_index >= 0) {
      input->inventory_action = SF_INVENTORY_ACTION_TAKE_SPECIAL;
      input->special_item_index = inventory->hovered_special_item_index;
      inventory->hovered_special_item_index = -1;
    }
    input->pointer_over_gameplay_ui = true;
    changed = true;
  } else if (input->pointer_secondary_pressed && belt_pocket && !holding) {
    input->inventory_action = SF_INVENTORY_ACTION_USE_BELT;
    input->belt_grid_x = (int8_t) belt_x;
    input->belt_grid_y = (int8_t) belt_y;
    input->pointer_over_gameplay_ui = true;
    changed = true;
  } else if (input->belt_pocket_key_pressed &&
             input->belt_pocket_pressed >= 0 &&
             input->belt_pocket_pressed < 8 && !holding) {
    input->inventory_action = SF_INVENTORY_ACTION_USE_BELT;
    input->belt_grid_x = (int8_t) (input->belt_pocket_pressed % 4);
    input->belt_grid_y = (int8_t) (input->belt_pocket_pressed / 4);
    changed = true;
  } else if (inventory->open && input->pointer_secondary_pressed &&
             !holding && inventory->hovered_item_index >= 0) {
    input->inventory_action = SF_INVENTORY_ACTION_USE_BACKPACK;
    input->inventory_item_index = inventory->hovered_item_index;
    input->pointer_over_gameplay_ui = true;
    changed = true;
  } else if (input->pointer_primary_pressed && holding &&
             input->pointer_y < 412 &&
             (!inventory->open ||
              input->pointer_x < SF_GAMEPLAY_INVENTORY_PANEL_LEFT) &&
             (!inventory->special_open ||
              input->pointer_x >= SF_GAMEPLAY_INVENTORY_PANEL_LEFT)) {
    input->inventory_action = SF_INVENTORY_ACTION_DROP_WORLD;
    input->pointer_over_gameplay_ui = true;
    changed = true;
  } else if (inventory->open && input->pointer_primary_pressed && holding &&
             equipment_slot < SF_EQUIPMENT_VISIBLE_SLOT_COUNT) {
    input->inventory_action = SF_INVENTORY_ACTION_PLACE_EQUIPMENT;
    input->equipment_slot = (int8_t) equipment_slot;
    input->pointer_over_gameplay_ui = true;
    changed = true;
  } else if (inventory->open && input->pointer_primary_pressed && !holding &&
             equipment_slot < SF_EQUIPMENT_VISIBLE_SLOT_COUNT &&
             sf_equipment_item(&player->equipment, equipment_slot)) {
    input->inventory_action = SF_INVENTORY_ACTION_TAKE_EQUIPMENT;
    input->equipment_slot = (int8_t) equipment_slot;
    inventory->hovered_equipment_slot = -1;
    input->pointer_over_gameplay_ui = true;
    changed = true;
  } else if (inventory->open && input->pointer_primary_pressed && holding &&
             sf_inventory_pointer_inside(
               input, SF_GAMEPLAY_INVENTORY_BACKPACK_LEFT,
               SF_GAMEPLAY_INVENTORY_BACKPACK_TOP,
               SF_GAMEPLAY_INVENTORY_BACKPACK_LEFT +
                 SF_INVENTORY_WIDTH * SF_GAMEPLAY_INVENTORY_CELL_SIZE,
               SF_GAMEPLAY_INVENTORY_BACKPACK_TOP +
                 SF_INVENTORY_HEIGHT * SF_GAMEPLAY_INVENTORY_CELL_SIZE)) {
    const SfInventoryItem *item = &player->inventory_transfer.held_item;
    input->inventory_action = SF_INVENTORY_ACTION_PLACE;
    input->inventory_grid_x = (int8_t) (
      (input->pointer_x - item->width * SF_GAMEPLAY_INVENTORY_CELL_SIZE / 2 -
       (SF_GAMEPLAY_INVENTORY_BACKPACK_LEFT -
        SF_GAMEPLAY_INVENTORY_CELL_SIZE / 2)) /
      SF_GAMEPLAY_INVENTORY_CELL_SIZE);
    input->inventory_grid_y = (int8_t) (
      (input->pointer_y - item->height * SF_GAMEPLAY_INVENTORY_CELL_SIZE / 2 -
       (SF_GAMEPLAY_INVENTORY_BACKPACK_TOP -
        SF_GAMEPLAY_INVENTORY_CELL_SIZE / 2)) /
      SF_GAMEPLAY_INVENTORY_CELL_SIZE);
    input->pointer_over_gameplay_ui = true;
    changed = true;
  } else if (inventory->open && input->pointer_primary_pressed && !holding &&
             inventory->hovered_item_index >= 0) {
    input->inventory_action = SF_INVENTORY_ACTION_TAKE;
    input->inventory_item_index = inventory->hovered_item_index;
    inventory->hovered_item_index = -1;
    input->pointer_over_gameplay_ui = true;
    changed = true;
  }
  input->world_view_offset_x = inventory->open
    ? SF_GAMEPLAY_INVENTORY_VIEW_OFFSET : 0;
  if (inventory->special_open)
    input->world_view_offset_x -= SF_GAMEPLAY_INVENTORY_VIEW_OFFSET;
  if (inventory->open && input->pointer_active &&
      input->pointer_x >= SF_GAMEPLAY_INVENTORY_PANEL_LEFT &&
      input->pointer_y < 412) input->pointer_over_gameplay_ui = true;
  if (inventory->special_open && input->pointer_active &&
      input->pointer_x < SF_GAMEPLAY_INVENTORY_PANEL_LEFT &&
      input->pointer_y < 412) input->pointer_over_gameplay_ui = true;
  if (holding && pointer_moved) changed = true;
  return changed;
}
