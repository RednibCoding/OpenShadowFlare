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

#ifndef SHADOWFLARE_GAME_PLAYER_SAVE_H
#define SHADOWFLARE_GAME_PLAYER_SAVE_H

#include "data/save_player.h"
#include "data/save_game.h"
#include "game/player.h"

#include <stdbool.h>
#include <stdint.h>

bool sf_player_restore_save(
  SfPlayerState *player, const SfSavedPlayer *saved,
  const SfItemGroundDefinition *definitions, uint8_t definition_count,
  int32_t experience_threshold);
bool sf_player_restore_magic(
  SfPlayerState *player, const SfSavedMagic *saved);
bool sf_player_restore_companions(
  SfPlayerState *player, const SfSavedPlayer *saved_player,
  const SfSavedCompanions *saved_companions);

#endif
