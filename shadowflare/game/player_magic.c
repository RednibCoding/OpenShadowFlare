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

#include "game/player_magic.h"

#include <string.h>

static bool sf_player_magic_spell_valid(int32_t spell) {
  return spell >= 0 && spell < (int32_t) SF_PLAYER_SPELL_COUNT;
}

void sf_player_magic_init(SfPlayerMagicState *magic) {
  uint8_t index;
  if (!magic) return;
  memset(magic, 0, sizeof(*magic));
  for (index = 0u; index < SF_PLAYER_SPELL_COUNT; ++index)
    magic->levels[index] = 1;
  for (index = 0u; index < SF_PLAYER_MAGIC_BAR_SLOT_COUNT; ++index)
    magic->bar_slots[index] = -1;
  magic->selected_spell = -1;
}

bool sf_player_magic_learned(
    const SfPlayerMagicState *magic, int32_t spell) {
  return magic && sf_player_magic_spell_valid(spell) &&
    magic->availability[spell] == 3;
}

int32_t sf_player_magic_bar_slot(
    const SfPlayerMagicState *magic, int32_t slot) {
  return magic && slot >= 0 &&
      slot < (int32_t) SF_PLAYER_MAGIC_BAR_SLOT_COUNT
    ? magic->bar_slots[slot] : -1;
}

bool sf_player_magic_assign(
    SfPlayerMagicState *magic, int32_t slot, int32_t spell) {
  uint8_t index;
  bool changed;
  if (!magic || slot < 0 ||
      slot >= (int32_t) SF_PLAYER_MAGIC_BAR_SLOT_COUNT ||
      !sf_player_magic_learned(magic, spell)) return false;
  changed = magic->bar_slots[slot] != spell;
  for (index = 0u; index < SF_PLAYER_MAGIC_BAR_SLOT_COUNT; ++index) {
    if (magic->bar_slots[index] != spell) continue;
    magic->bar_slots[index] = -1;
    changed = true;
  }
  magic->bar_slots[slot] = spell;
  return changed;
}

bool sf_player_magic_select(SfPlayerMagicState *magic, int32_t spell) {
  if (!magic || (spell != -1 && !sf_player_magic_learned(magic, spell)) ||
      magic->selected_spell == spell) return false;
  magic->selected_spell = (int8_t) spell;
  if (spell >= 0) magic->targeting = false;
  return true;
}

bool sf_player_magic_set_targeting(
    SfPlayerMagicState *magic, bool targeting) {
  if (!magic || (magic->targeting == targeting &&
      (!targeting || magic->selected_spell == -1))) return false;
  magic->targeting = targeting;
  if (targeting) magic->selected_spell = -1;
  return true;
}
