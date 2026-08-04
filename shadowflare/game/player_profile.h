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

#ifndef SHADOWFLARE_GAME_PLAYER_PROFILE_H
#define SHADOWFLARE_GAME_PLAYER_PROFILE_H

#include "data/item.h"
#include "game/player.h"

#include <stdint.h>

typedef struct SfPlayerProfile {
  int32_t attack_speed;
  int32_t walking_speed;
  int32_t maximum_life;
  int32_t maximum_mana;
  int32_t weight_capacity;
  int32_t physical_attack;
  int32_t physical_defense;
  int32_t hit_rate;
  int32_t physical_evasion;
  int32_t magical_attack;
  int32_t magical_defense;
  int32_t magical_hit_rate;
  int32_t magical_evasion;
} SfPlayerProfile;

void sf_player_profile_build(
  const SfPlayerState *player,
  const SfItemGroundDefinition *definitions, uint8_t definition_count,
  SfPlayerProfile *profile);
const char *sf_player_job_name(int32_t job, uint8_t gender);

#endif
