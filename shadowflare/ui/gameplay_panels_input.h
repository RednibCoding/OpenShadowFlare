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

#ifndef SHADOWFLARE_UI_GAMEPLAY_PANELS_INPUT_H
#define SHADOWFLARE_UI_GAMEPLAY_PANELS_INPUT_H

#include "game/input.h"
#include "game/player.h"
#include "ui/gameplay_inventory.h"
#include "ui/gameplay_character_panel.h"

#include <stdbool.h>

bool sf_gameplay_panels_input_resolve(
  SfGameplayCharacterPanelUi *character, SfGameplayInventoryUi *inventory,
  const SfPlayerState *player, bool conversation_active,
  SfGameInput *input);

#endif
