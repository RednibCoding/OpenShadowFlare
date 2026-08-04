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

#include "ui/gameplay_magic_layout.h"

#include <string.h>

bool sf_gameplay_magic_region_contains(
    SfGameplayMagicRegion region, int32_t x, int32_t y) {
  return region.width > 0 && region.height > 0 && x >= region.x &&
    x < region.x + region.width && y >= region.y &&
    y < region.y + region.height;
}

void sf_gameplay_magic_bar_layout(
    const SfPlayerMagicState *magic, bool left_panel, bool right_panel,
    SfGameplayMagicRegion slots[SF_PLAYER_MAGIC_BAR_SLOT_COUNT],
    SfGameplayMagicRegion *target) {
  int16_t x;
  uint8_t slot;
  if (!slots || !target) return;
  memset(slots, 0,
    sizeof(*slots) * SF_PLAYER_MAGIC_BAR_SLOT_COUNT);
  *target = (SfGameplayMagicRegion) {0, 0, 0, 0};
  if (!magic || (left_panel && right_panel)) return;
  x = left_panel ? 344 : right_panel ? 124 : 224;
  for (slot = 0u; slot < SF_PLAYER_MAGIC_BAR_SLOT_COUNT; ++slot) {
    const bool selected = magic->bar_slots[slot] >= 0 &&
      magic->bar_slots[slot] == magic->selected_spell;
    const int16_t size = selected ? 26 : 16;
    if (slot % 4u == 0u) x += 4;
    slots[slot] = (SfGameplayMagicRegion) {
      x, (int16_t) (selected ? 382 : 392), size, size};
    x = (int16_t) (x + size);
  }
  x = (int16_t) (x + 4);
  *target = (SfGameplayMagicRegion) {
    x, (int16_t) (magic->targeting ? 392 : 382),
    (int16_t) (magic->targeting ? 16 : 26),
    (int16_t) (magic->targeting ? 16 : 26)};
}
