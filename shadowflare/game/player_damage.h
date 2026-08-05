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

#ifndef SHADOWFLARE_GAME_PLAYER_DAMAGE_H
#define SHADOWFLARE_GAME_PLAYER_DAMAGE_H

#include "data/combat_tables.h"
#include "data/item.h"
#include "game/combat_packet.h"
#include "game/player.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct SfPlayerDamageResult {
  int32_t damage;
  uint16_t audio_sample;
  bool valid;
  bool accepted;
  bool revived;
} SfPlayerDamageResult;

void sf_player_combat_defense(
  const SfPlayerState *player,
  const SfItemGroundDefinition *definitions, uint8_t definition_count,
  SfCombatDefense *defense, int32_t *physical_evasion);
SfPlayerDamageResult sf_player_receive_damage(
  SfPlayerState *player, const SfCombatPacket *packet,
  SfWorldPoint impact_origin,
  const SfItemGroundDefinition *definitions, uint8_t definition_count,
  const SfCombatTables *tables, uint32_t *random_state);

#endif
