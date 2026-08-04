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

#include "ui/gameplay_service_controller.h"

bool sf_gameplay_service_apply(
    SfGameplayCharacterPanelUi *character,
    SfGameplayInventoryUi *inventory,
    SfGameplayServiceRequest request) {
  if (!character || !inventory) return false;
  if (request.kind == SF_GAMEPLAY_SERVICE_NONE) return true;
  if (request.kind != SF_GAMEPLAY_SERVICE_TOGGLE_SPECIAL_ITEMS ||
      request.argument != 0) return false;
  character->tab = SF_GAMEPLAY_CHARACTER_TAB_CLOSED;
  character->held_spell = -1;
  inventory->special_open = !inventory->special_open;
  inventory->close_hovered = false;
  inventory->hovered_special_item_index = -1;
  inventory->item_hover_updates = 0u;
  return true;
}
