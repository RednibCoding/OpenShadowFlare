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

#include "ui/gameplay_hud_input.h"

static bool sf_inventory_pointer_inside(
    const SfGameInput *input, int left, int top, int right, int bottom) {
  return input->pointer_active && input->pointer_x >= left &&
    input->pointer_x < right && input->pointer_y >= top &&
    input->pointer_y < bottom;
}

bool sf_gameplay_inventory_input_resolve(
    SfGameplayInventoryUi *inventory, bool conversation_active,
    SfGameInput *input) {
  bool changed = false;
  bool toggle;
  bool close_hovered;
  if (!inventory || !input) return false;
  sf_gameplay_hud_input_resolve(input);
  toggle = input->inventory_pressed ||
    sf_gameplay_hud_button_at_pointer(input) ==
      SF_GAMEPLAY_HUD_BUTTON_INVENTORY;
  if (toggle && (!conversation_active || inventory->open)) {
    inventory->open = !inventory->open;
    changed = true;
  }
  close_hovered = inventory->open && sf_inventory_pointer_inside(
    input, 375, 393, 443, 404);
  if (inventory->close_hovered != close_hovered) {
    inventory->close_hovered = close_hovered;
    changed = true;
  }
  if (inventory->open && input->cancel_pressed) {
    inventory->open = false;
    inventory->close_hovered = false;
    input->cancel_pressed = false;
    changed = true;
  } else if (inventory->open && inventory->close_hovered &&
             input->pointer_primary_pressed) {
    inventory->open = false;
    inventory->close_hovered = false;
    changed = true;
  }
  input->world_view_offset_x = inventory->open
    ? SF_GAMEPLAY_INVENTORY_VIEW_OFFSET : 0;
  if (inventory->open && input->pointer_active &&
      input->pointer_x >= SF_GAMEPLAY_INVENTORY_PANEL_LEFT &&
      input->pointer_y < 412) input->pointer_over_gameplay_ui = true;
  return changed;
}
