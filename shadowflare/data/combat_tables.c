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

#include "data/combat_tables.h"

#include "data/table.h"

#include <string.h>

static bool sf_combat_table_cell(
    void *user, int32_t table, int32_t row,
    int32_t column, int32_t value) {
  SfCombatTables *tables = (SfCombatTables *) user;
  if (row < 0 || column < 0) return true;
  if (table == SF_COMBAT_TABLE_PHYSICAL_SCALE &&
      row < (int32_t) SF_COMBAT_SCALE_ROW_COUNT &&
      column == 0) {
    tables->physical_scale[row] = value;
    tables->physical_scale_present[row] = 1u;
  } else if (table == SF_COMBAT_TABLE_DEFENSE_SCALE &&
             row < (int32_t) SF_COMBAT_SCALE_ROW_COUNT &&
             column == 0) {
    tables->defense_scale[row] = value;
    tables->defense_scale_present[row] = 1u;
  } else if (table == SF_COMBAT_TABLE_COMPANION_AFFINITY &&
             row < (int32_t) SF_COMBAT_AFFINITY_ROW_COUNT && column < 2) {
    tables->companion_affinity[row][column] = value;
    tables->companion_affinity_present[row][column] = 1u;
  } else if (table == SF_COMBAT_TABLE_REACTION &&
             row < (int32_t) SF_COMBAT_REACTION_ROW_COUNT && column < 2) {
    tables->reaction[row][column] = value;
    tables->reaction_present[row][column] = 1u;
  } else if (table == SF_COMBAT_TABLE_PLAYER_AFFINITY &&
             row < (int32_t) SF_COMBAT_AFFINITY_ROW_COUNT && column < 2) {
    tables->player_affinity[row][column] = value;
    tables->player_affinity_present[row][column] = 1u;
  }
  return true;
}

bool sf_combat_tables_load(const char *path, SfCombatTables *tables) {
  if (!path || !tables) return false;
  memset(tables, 0, sizeof(*tables));
  return sf_table_scan_numeric(path, sf_combat_table_cell, tables);
}

bool sf_combat_table_value(
    const SfCombatTables *tables, int32_t table, int32_t row,
    int32_t column, int32_t *value) {
  if (!tables || !value || row < 0 || column < 0) return false;
  if (table == SF_COMBAT_TABLE_PHYSICAL_SCALE &&
      row < (int32_t) SF_COMBAT_SCALE_ROW_COUNT &&
      column == 0 && tables->physical_scale_present[row])
    *value = tables->physical_scale[row];
  else if (table == SF_COMBAT_TABLE_DEFENSE_SCALE &&
           row < (int32_t) SF_COMBAT_SCALE_ROW_COUNT &&
           column == 0 && tables->defense_scale_present[row])
    *value = tables->defense_scale[row];
  else if (table == SF_COMBAT_TABLE_COMPANION_AFFINITY &&
           row < (int32_t) SF_COMBAT_AFFINITY_ROW_COUNT &&
           column < 2 && tables->companion_affinity_present[row][column])
    *value = tables->companion_affinity[row][column];
  else if (table == SF_COMBAT_TABLE_REACTION &&
           row < (int32_t) SF_COMBAT_REACTION_ROW_COUNT &&
           column < 2 && tables->reaction_present[row][column])
    *value = tables->reaction[row][column];
  else if (table == SF_COMBAT_TABLE_PLAYER_AFFINITY &&
           row < (int32_t) SF_COMBAT_AFFINITY_ROW_COUNT &&
           column < 2 && tables->player_affinity_present[row][column])
    *value = tables->player_affinity[row][column];
  else return false;
  return true;
}
