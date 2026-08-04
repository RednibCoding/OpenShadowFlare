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

#ifndef SHADOWFLARE_DATA_COMBAT_TABLES_H
#define SHADOWFLARE_DATA_COMBAT_TABLES_H

#include <stdbool.h>
#include <stdint.h>

#define SF_COMBAT_SCALE_ROW_COUNT 21u
#define SF_COMBAT_AFFINITY_ROW_COUNT 10u
#define SF_COMBAT_REACTION_ROW_COUNT 50u

typedef enum SfCombatTableNumber {
  SF_COMBAT_TABLE_PHYSICAL_SCALE = 7,
  SF_COMBAT_TABLE_DEFENSE_SCALE = 11,
  SF_COMBAT_TABLE_COMPANION_AFFINITY = 24,
  SF_COMBAT_TABLE_REACTION = 25,
  SF_COMBAT_TABLE_PLAYER_AFFINITY = 26
} SfCombatTableNumber;

typedef struct SfCombatTables {
  int32_t physical_scale[SF_COMBAT_SCALE_ROW_COUNT];
  int32_t defense_scale[SF_COMBAT_SCALE_ROW_COUNT];
  int32_t companion_affinity[SF_COMBAT_AFFINITY_ROW_COUNT][2];
  int32_t reaction[SF_COMBAT_REACTION_ROW_COUNT][2];
  int32_t player_affinity[SF_COMBAT_AFFINITY_ROW_COUNT][2];
  uint8_t physical_scale_present[SF_COMBAT_SCALE_ROW_COUNT];
  uint8_t defense_scale_present[SF_COMBAT_SCALE_ROW_COUNT];
  uint8_t companion_affinity_present[SF_COMBAT_AFFINITY_ROW_COUNT][2];
  uint8_t reaction_present[SF_COMBAT_REACTION_ROW_COUNT][2];
  uint8_t player_affinity_present[SF_COMBAT_AFFINITY_ROW_COUNT][2];
} SfCombatTables;

bool sf_combat_tables_load(const char *path, SfCombatTables *tables);
bool sf_combat_table_value(
  const SfCombatTables *tables, int32_t table, int32_t row,
  int32_t column, int32_t *value);

#endif
