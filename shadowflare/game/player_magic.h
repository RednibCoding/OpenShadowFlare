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

#ifndef SHADOWFLARE_GAME_PLAYER_MAGIC_H
#define SHADOWFLARE_GAME_PLAYER_MAGIC_H

#include <stdbool.h>
#include <stdint.h>

#define SF_PLAYER_SPELL_COUNT 22u
#define SF_PLAYER_MAGIC_BAR_SLOT_COUNT 8u

typedef struct SfPlayerMagicState {
  int32_t availability[SF_PLAYER_SPELL_COUNT];
  int32_t levels[SF_PLAYER_SPELL_COUNT];
  int32_t experience[SF_PLAYER_SPELL_COUNT];
  int32_t bar_slots[SF_PLAYER_MAGIC_BAR_SLOT_COUNT];
  int8_t selected_spell;
  bool targeting;
} SfPlayerMagicState;

void sf_player_magic_init(SfPlayerMagicState *magic);
bool sf_player_magic_learned(
  const SfPlayerMagicState *magic, int32_t spell);
int32_t sf_player_magic_bar_slot(
  const SfPlayerMagicState *magic, int32_t slot);
bool sf_player_magic_assign(
  SfPlayerMagicState *magic, int32_t slot, int32_t spell);
bool sf_player_magic_select(SfPlayerMagicState *magic, int32_t spell);
bool sf_player_magic_set_targeting(
  SfPlayerMagicState *magic, bool targeting);

#endif
