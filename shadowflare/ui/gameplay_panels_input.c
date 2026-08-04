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

#include "ui/gameplay_panels_input.h"

#include "ui/gameplay_hud_input.h"
#include "ui/gameplay_inventory_input.h"
#include "ui/gameplay_magic_input.h"

bool sf_gameplay_panels_input_resolve(
    SfGameplayCharacterPanelUi *character, SfGameplayInventoryUi *inventory,
    const SfPlayerState *player, bool conversation_active,
    SfGameInput *input) {
  const SfGameplayHudButton hud_button =
    sf_gameplay_hud_button_at_pointer(input);
  const bool status_toggle = input &&
    (input->status_pressed || hud_button == SF_GAMEPLAY_HUD_BUTTON_STATUS ||
     (character && character->tab == SF_GAMEPLAY_CHARACTER_TAB_MAGIC &&
      input->pointer_active && input->pointer_primary_pressed &&
      input->pointer_x >= 0 && input->pointer_x < 160 &&
      input->pointer_y >= 0 && input->pointer_y < 37));
  const bool magic_toggle = input &&
    (input->magic_pressed ||
     (character && character->tab == SF_GAMEPLAY_CHARACTER_TAB_STATUS &&
      input->pointer_active && input->pointer_primary_pressed &&
      input->pointer_x >= 160 && input->pointer_x < 320 &&
      input->pointer_y >= 0 && input->pointer_y < 37));
  bool status_pointer;
  bool primary_pressed;
  bool secondary_pressed;
  bool changed = false;
  if (!character || !inventory || !player || !input) return false;
  if (input->cancel_pressed &&
      (character->tab != SF_GAMEPLAY_CHARACTER_TAB_CLOSED ||
       inventory->open || inventory->special_open)) {
    character->tab = SF_GAMEPLAY_CHARACTER_TAB_CLOSED;
    character->held_spell = -1;
    inventory->open = false;
    inventory->special_open = false;
    inventory->close_hovered = false;
    inventory->hovered_special_item_index = -1;
    input->cancel_pressed = false;
    changed = true;
  } else {
    if (status_toggle && (!conversation_active ||
          character->tab == SF_GAMEPLAY_CHARACTER_TAB_STATUS)) {
      character->tab = character->tab == SF_GAMEPLAY_CHARACTER_TAB_STATUS
        ? SF_GAMEPLAY_CHARACTER_TAB_CLOSED
        : SF_GAMEPLAY_CHARACTER_TAB_STATUS;
      character->held_spell = -1;
      if (character->tab != SF_GAMEPLAY_CHARACTER_TAB_CLOSED)
        inventory->special_open = false;
      if (hud_button == SF_GAMEPLAY_HUD_BUTTON_STATUS)
        input->pointer_over_gameplay_ui = true;
      changed = true;
    }
    if (magic_toggle && (!conversation_active ||
          character->tab == SF_GAMEPLAY_CHARACTER_TAB_MAGIC)) {
      character->tab = character->tab == SF_GAMEPLAY_CHARACTER_TAB_MAGIC
        ? SF_GAMEPLAY_CHARACTER_TAB_CLOSED
        : SF_GAMEPLAY_CHARACTER_TAB_MAGIC;
      character->magic_page = 0u;
      character->held_spell = -1;
      if (character->tab != SF_GAMEPLAY_CHARACTER_TAB_CLOSED)
        inventory->special_open = false;
      if (input->pointer_active && input->pointer_primary_pressed)
        input->pointer_over_gameplay_ui = true;
      input->interface_sound = 58u;
      changed = true;
    }
    if (input->special_items_pressed &&
        character->tab != SF_GAMEPLAY_CHARACTER_TAB_CLOSED &&
        (!conversation_active || inventory->special_open)) {
      character->tab = SF_GAMEPLAY_CHARACTER_TAB_CLOSED;
      character->held_spell = -1;
      changed = true;
    }
  }
  status_pointer = character->tab != SF_GAMEPLAY_CHARACTER_TAB_CLOSED &&
    input->pointer_active &&
    input->pointer_x < 320 && input->pointer_y < 412;
  primary_pressed = input->pointer_primary_pressed;
  secondary_pressed = input->pointer_secondary_pressed;
  if (sf_gameplay_magic_input_resolve(
        character, player,
        character->tab != SF_GAMEPLAY_CHARACTER_TAB_CLOSED ||
          inventory->special_open,
        inventory->open, input)) changed = true;
  if (status_pointer) {
    input->pointer_primary_pressed = false;
    input->pointer_secondary_pressed = false;
  }
  if (sf_gameplay_inventory_input_resolve(
        inventory, player, conversation_active, input)) changed = true;
  input->pointer_primary_pressed = primary_pressed;
  input->pointer_secondary_pressed = secondary_pressed;
  if (character->tab != SF_GAMEPLAY_CHARACTER_TAB_CLOSED) {
    input->world_view_offset_x -= SF_GAMEPLAY_INVENTORY_VIEW_OFFSET;
    if (status_pointer) input->pointer_over_gameplay_ui = true;
  }
  return changed;
}
