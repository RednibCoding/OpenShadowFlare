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

#ifndef SHADOWFLARE_GAME_COMPANION_DAMAGE_H
#define SHADOWFLARE_GAME_COMPANION_DAMAGE_H

#include "data/combat_tables.h"
#include "game/combat_packet.h"
#include "game/companion.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct SfCompanionDamageResult {
  int32_t damage;
  bool valid;
  bool accepted;
} SfCompanionDamageResult;

void sf_companion_combat_defense(
  const SfCompanionState *companion,
  SfCombatDefense *defense, int32_t *physical_evasion);
SfCompanionDamageResult sf_companion_receive_damage(
  SfCompanionState *companion, const SfCombatPacket *packet,
  SfWorldPoint impact_origin, const SfCombatTables *tables,
  uint32_t *random_state);

#endif
