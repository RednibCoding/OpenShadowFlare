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

#include "game/world_magic.h"

void sf_world_magic_update(
    SfPlayerState *player, const SfGameInput *input) {
  if (!player || !input) return;
  if (input->magic_action == SF_MAGIC_ACTION_ASSIGN)
    (void) sf_player_magic_assign(
      &player->magic, input->magic_bar_slot, input->magic_spell);
  else if (input->magic_action == SF_MAGIC_ACTION_SELECT)
    (void) sf_player_magic_select(&player->magic, input->magic_spell);
  else if (input->magic_action == SF_MAGIC_ACTION_TOGGLE_TARGETING)
    (void) sf_player_magic_set_targeting(
      &player->magic, !player->magic.targeting);
}
