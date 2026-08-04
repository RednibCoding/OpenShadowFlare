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

#ifndef SHADOWFLARE_UI_GAMEPLAY_HUD_INPUT_H
#define SHADOWFLARE_UI_GAMEPLAY_HUD_INPUT_H

#include "game/input.h"

typedef enum SfGameplayHudButton {
  SF_GAMEPLAY_HUD_BUTTON_NONE = 0,
  SF_GAMEPLAY_HUD_BUTTON_OPTIONS,
  SF_GAMEPLAY_HUD_BUTTON_STATUS,
  SF_GAMEPLAY_HUD_BUTTON_INVENTORY
} SfGameplayHudButton;

void sf_gameplay_hud_input_resolve(SfGameInput *input);
SfGameplayHudButton sf_gameplay_hud_button_at_pointer(
  const SfGameInput *input);

#endif
