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

#ifndef SHADOWFLARE_GAME_INPUT_H
#define SHADOWFLARE_GAME_INPUT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum SfInventoryAction {
  SF_INVENTORY_ACTION_NONE = 0,
  SF_INVENTORY_ACTION_TAKE,
  SF_INVENTORY_ACTION_PLACE,
  SF_INVENTORY_ACTION_TAKE_EQUIPMENT,
  SF_INVENTORY_ACTION_PLACE_EQUIPMENT,
  SF_INVENTORY_ACTION_TAKE_BELT,
  SF_INVENTORY_ACTION_PLACE_BELT,
  SF_INVENTORY_ACTION_TAKE_SPECIAL,
  SF_INVENTORY_ACTION_PLACE_SPECIAL,
  SF_INVENTORY_ACTION_USE_BACKPACK,
  SF_INVENTORY_ACTION_USE_BELT,
  SF_INVENTORY_ACTION_DROP_WORLD
} SfInventoryAction;

typedef enum SfMagicAction {
  SF_MAGIC_ACTION_NONE = 0,
  SF_MAGIC_ACTION_ASSIGN,
  SF_MAGIC_ACTION_SELECT,
  SF_MAGIC_ACTION_TOGGLE_TARGETING
} SfMagicAction;

typedef struct SfGameInput {
  int32_t pointed_actor_id;
  int32_t pointed_ground_item_id;
  int8_t pointed_conversation_option;
  int16_t pointer_x;
  int16_t pointer_y;
  int16_t world_view_offset_x;
  int8_t inventory_item_index;
  int8_t inventory_grid_x;
  int8_t inventory_grid_y;
  int8_t equipment_slot;
  int8_t belt_grid_x;
  int8_t belt_grid_y;
  int8_t special_item_index;
  int8_t special_grid_x;
  int8_t special_grid_y;
  int8_t belt_pocket_pressed;
  uint8_t conversation_option_count;
  SfInventoryAction inventory_action;
  SfMagicAction magic_action;
  int8_t magic_spell;
  int8_t magic_bar_slot;
  uint16_t interface_sound;
  bool pointer_active;
  bool pointer_over_gameplay_ui;
  bool world_pointer_resolved;
  bool conversation_choices_resolved;
  bool pointer_primary_pressed;
  bool pointer_primary_down;
  bool pointer_secondary_pressed;
  bool up_pressed;
  bool down_pressed;
  bool left_pressed;
  bool right_pressed;
  bool confirm_pressed;
  bool cancel_pressed;
  bool backspace_pressed;
  bool delete_pressed;
  bool pace_toggle_pressed;
  bool inventory_pressed;
  bool status_pressed;
  bool magic_pressed;
  bool special_items_pressed;
  bool belt_pocket_key_pressed;
  char text[16];
  uint8_t text_length;
} SfGameInput;

#endif
