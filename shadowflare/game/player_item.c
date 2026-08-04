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

#include "game/player_item.h"

#include <stdint.h>

static int32_t sf_player_restored_value(
    int32_t current, int32_t maximum,
    int32_t flat_amount, int32_t maximum_percent) {
  int64_t result;
  if (maximum <= 0) return current;
  result = (int64_t) current + flat_amount +
    (int64_t) maximum_percent * maximum / 100;
  if (result < 0) return 0;
  if (result > maximum) return maximum;
  return (int32_t) result;
}

bool sf_player_use_medicine(
    SfPlayerState *player, const SfItemGroundDefinition *definition) {
  int32_t life;
  int32_t mana;
  if (!player || !definition || definition->category != 3u) return false;
  life = sf_player_restored_value(
    player->current_life, player->initial_parameters.values[2],
    definition->restore_life, definition->restore_life_percent);
  mana = sf_player_restored_value(
    player->current_mana, player->initial_parameters.values[3],
    definition->restore_mana, definition->restore_mana_percent);
  if (life == player->current_life && mana == player->current_mana)
    return false;
  player->current_life = life;
  player->current_mana = mana;
  return true;
}

bool sf_player_collect_mine(SfPlayerState *player) {
  if (!player || player->mine_count >= player->maximum_mines) return false;
  ++player->mine_count;
  return true;
}
