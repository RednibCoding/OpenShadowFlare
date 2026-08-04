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

#include "data/player_parameters.h"

#include "data/table.h"

#include <string.h>

typedef struct SfPlayerParameterScan {
  SfPlayerInitialParameters *parameters;
  int32_t table;
  int32_t threshold_row;
  uint16_t found;
} SfPlayerParameterScan;

static bool sf_player_parameter_value(
    void *user, int32_t table, int32_t row,
    int32_t column, int32_t value) {
  SfPlayerParameterScan *scan = (SfPlayerParameterScan *) user;
  if (table == scan->table && column == 0 && row >= 0 &&
      row < (int32_t) SF_PLAYER_INITIAL_PARAMETER_COUNT) {
    scan->parameters->values[row] = value;
    scan->found = (uint16_t) (scan->found | (uint16_t) (1u << row));
  }
  if (table == 13 && row == scan->threshold_row && column == 0) {
    scan->parameters->experience_threshold = value;
    scan->found = (uint16_t) (scan->found | (uint16_t) (1u << 13u));
  }
  return true;
}

bool sf_player_initial_parameters_load(
    const char *path, uint8_t gender, int32_t level,
    SfPlayerInitialParameters *parameters) {
  SfPlayerParameterScan scan;
  if (!path || !parameters || level <= 0 || level > 100) return false;
  memset(parameters, 0, sizeof(*parameters));
  scan.parameters = parameters;
  scan.table = gender == 1u ? 900 : 901;
  scan.threshold_row = level < 100 ? level - 1 : -1;
  scan.found = level < 100 ? 0u : (uint16_t) (1u << 13u);
  return sf_table_scan_numeric(path, sf_player_parameter_value, &scan) &&
    scan.found == 0x3fffu;
}
