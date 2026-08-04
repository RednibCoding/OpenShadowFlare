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

#ifndef SHADOWFLARE_GAME_PLAYER_ELEMENTS_H
#define SHADOWFLARE_GAME_PLAYER_ELEMENTS_H

#include "data/item.h"
#include "game/player.h"

#include <stdint.h>

#define SF_PLAYER_ELEMENT_COUNT 8u

void sf_player_element_affinities(
  const SfPlayerState *player,
  const SfItemGroundDefinition *definitions, uint8_t definition_count,
  int8_t affinities[SF_PLAYER_ELEMENT_COUNT]);

#endif
