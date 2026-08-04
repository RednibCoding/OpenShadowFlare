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

#include "data/companion_parameters.h"

#include "data/table.h"

#include <string.h>

typedef struct SfCompanionProfileScan {
  SfCompanionProfile *profile;
  uint32_t parameter_cells;
  uint8_t catalog_values;
  bool name_found;
} SfCompanionProfileScan;

static int32_t sf_companion_parameter_add(int32_t first, int32_t second) {
  return (int32_t) ((uint32_t) first + (uint32_t) second);
}

static bool sf_companion_profile_value(
    void *user, int32_t table, int32_t row,
    int32_t column, int32_t value) {
  SfCompanionProfileScan *scan = (SfCompanionProfileScan *) user;
  SfCompanionProfile *profile = scan->profile;
  if (table == 60 && row == profile->type && column >= 1 && column <= 4) {
    if (column == 1) profile->resource_id = value;
    if (column == 2) profile->red_strength = value;
    if (column == 3) profile->green_strength = value;
    if (column == 4) profile->blue_strength = value;
    scan->catalog_values = (uint8_t) (
      scan->catalog_values | (uint8_t) (1u << (uint8_t) (column - 1)));
  }
  if (table == 800 + profile->type && row >= 0 &&
      row < (int32_t) SF_COMPANION_PARAMETER_COUNT &&
      column >= 0 && column < profile->level) {
    profile->values[row] = sf_companion_parameter_add(
      profile->values[row], value);
    ++scan->parameter_cells;
  }
  return true;
}

static bool sf_companion_profile_text(
    void *user, int32_t table, int32_t row, int32_t column,
    uint32_t byte_index, uint32_t byte_count, uint8_t value) {
  SfCompanionProfileScan *scan = (SfCompanionProfileScan *) user;
  if (table != 60 || row != scan->profile->type || column != 0)
    return true;
  if (byte_index + 1u < SF_COMPANION_NAME_CAPACITY)
    scan->profile->name[byte_index] = (char) value;
  if (byte_index + 1u == byte_count) scan->name_found = byte_count > 0u;
  return true;
}

bool sf_companion_profile_load(
    const char *path, int32_t type, int32_t level,
    SfCompanionProfile *profile) {
  SfCompanionProfileScan scan;
  if (!path || !profile || type < 0 || type >= (int32_t) SF_COMPANION_COUNT ||
      level < 1 || level > 35) return false;
  memset(profile, 0, sizeof(*profile));
  profile->type = type;
  profile->level = level;
  scan.profile = profile;
  scan.parameter_cells = 0u;
  scan.catalog_values = 0u;
  scan.name_found = false;
  if (!sf_table_scan(
        path, sf_companion_profile_value, &scan,
        sf_companion_profile_text, &scan) ||
      scan.catalog_values != 0x0fu || !scan.name_found ||
      scan.parameter_cells != SF_COMPANION_PARAMETER_COUNT * (uint32_t) level ||
      profile->name[0] == '\0' || profile->resource_id < 0 ||
      profile->values[3] < 1) {
    memset(profile, 0, sizeof(*profile));
    return false;
  }
  return true;
}
