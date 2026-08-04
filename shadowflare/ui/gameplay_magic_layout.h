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

#ifndef SHADOWFLARE_UI_GAMEPLAY_MAGIC_LAYOUT_H
#define SHADOWFLARE_UI_GAMEPLAY_MAGIC_LAYOUT_H

#include "game/player_magic.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct SfGameplayMagicRegion {
  int16_t x;
  int16_t y;
  int16_t width;
  int16_t height;
} SfGameplayMagicRegion;

bool sf_gameplay_magic_region_contains(
  SfGameplayMagicRegion region, int32_t x, int32_t y);
void sf_gameplay_magic_bar_layout(
  const SfPlayerMagicState *magic, bool left_panel, bool right_panel,
  SfGameplayMagicRegion slots[SF_PLAYER_MAGIC_BAR_SLOT_COUNT],
  SfGameplayMagicRegion *target);

#endif
