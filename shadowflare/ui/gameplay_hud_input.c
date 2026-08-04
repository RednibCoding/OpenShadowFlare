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

#include "ui/gameplay_hud_input.h"

static bool sf_hud_inside_inclusive(
    int x, int y, int left, int top, int right, int bottom) {
  return x >= left && x <= right && y >= top && y <= bottom;
}

void sf_gameplay_hud_input_resolve(SfGameInput *input) {
  if (!input) return;
  input->pointer_over_gameplay_ui =
    input->pointer_active && input->pointer_y >= 393;
}

SfGameplayHudButton sf_gameplay_hud_button_at_pointer(
    const SfGameInput *input) {
  if (!input || !input->pointer_active ||
      !input->pointer_primary_pressed) return SF_GAMEPLAY_HUD_BUTTON_NONE;
  if (sf_hud_inside_inclusive(
        input->pointer_x, input->pointer_y, 589, 402, 639, 413))
    return SF_GAMEPLAY_HUD_BUTTON_OPTIONS;
  if (sf_hud_inside_inclusive(
        input->pointer_x, input->pointer_y, 537, 420, 577, 437))
    return SF_GAMEPLAY_HUD_BUTTON_STATUS;
  if (sf_hud_inside_inclusive(
        input->pointer_x, input->pointer_y, 583, 429, 636, 448))
    return SF_GAMEPLAY_HUD_BUTTON_INVENTORY;
  return SF_GAMEPLAY_HUD_BUTTON_NONE;
}
