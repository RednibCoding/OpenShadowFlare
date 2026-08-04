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

#include "data/item.h"

#include "data/item_records.h"

typedef struct SfItemAppearanceScan {
  SfItemAppearance appearance;
  int32_t target_id;
  int32_t record_id;
  uint16_t appearance_offset;
  uint8_t target_category;
  bool found;
} SfItemAppearanceScan;

static bool sf_item_appearance_word(
    void *user, uint8_t category, uint16_t offset, int32_t value) {
  SfItemAppearanceScan *scan = (SfItemAppearanceScan *) user;
  if (offset == 4u) scan->record_id = value;
  if (category != scan->target_category ||
      scan->record_id != scan->target_id) return true;
  if (offset == scan->appearance_offset) scan->appearance.part = value;
  if (offset == scan->appearance_offset + 4u) scan->appearance.red = value;
  if (offset == scan->appearance_offset + 8u) scan->appearance.green = value;
  if (offset == scan->appearance_offset + 12u) {
    scan->appearance.blue = value;
    scan->found = true;
  }
  return true;
}

bool sf_item_read_appearance(
    const char *path, uint8_t category, int32_t definition_id,
    SfItemAppearance *appearance) {
  SfItemAppearanceScan scan;
  if (!path || category > 1u || !appearance) return false;
  scan = (SfItemAppearanceScan) {
    {-1, 1000, 1000, 1000}, definition_id, -1,
    category == 0u ? 168u : 152u, category, false};
  if (!sf_item_scan_records(path, sf_item_appearance_word, &scan) ||
      !scan.found) return false;
  *appearance = scan.appearance;
  return true;
}
