/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of OpenShadowFlare.
 *
 * OpenShadowFlare is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenShadowFlare is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with OpenShadowFlare. If not, see <https://www.gnu.org/licenses/>.
 */

#include "data/transport.h"

#include "data/table.h"

#include <string.h>

typedef struct SfTransportScan {
  SfTransportCatalog *catalog;
  uint8_t fields[SF_TRANSPORT_DESTINATION_COUNT];
} SfTransportScan;

static bool sf_transport_number(
    void *user, int32_t table, int32_t row,
    int32_t column, int32_t value) {
  SfTransportScan *scan = (SfTransportScan *) user;
  SfTransportDestination *destination;
  if (table != 40) return true;
  if (row < 0 || row >= (int32_t) SF_TRANSPORT_DESTINATION_COUNT ||
      column < 0 || column > 2) return false;
  destination = &scan->catalog->destinations[row];
  if (column == 1) {
    destination->scenario_id = value;
    scan->fields[row] |= 1u;
  } else if (column == 2) {
    destination->entry_value = value;
    scan->fields[row] |= 2u;
  }
  if (scan->catalog->count <= (uint8_t) row)
    scan->catalog->count = (uint8_t) row + 1u;
  return true;
}

static bool sf_transport_text(
    void *user, int32_t table, int32_t row, int32_t column,
    uint32_t byte_index, uint32_t byte_count, uint8_t value) {
  SfTransportScan *scan = (SfTransportScan *) user;
  char *name;
  if (table != 40 || column != 0) return true;
  if (row < 0 || row >= (int32_t) SF_TRANSPORT_DESTINATION_COUNT ||
      byte_count == 0u || byte_count >= SF_TRANSPORT_NAME_CAPACITY)
    return false;
  if (byte_index >= byte_count) return false;
  name = scan->catalog->destinations[row].name;
  name[byte_index] = (char) value;
  if (byte_index + 1u == byte_count) {
    name[byte_count] = '\0';
    scan->fields[row] |= 4u;
  }
  return true;
}

bool sf_transport_catalog_load(
    const char *table_path, SfTransportCatalog *catalog) {
  SfTransportScan scan;
  uint8_t row;
  if (!table_path || !catalog) return false;
  memset(catalog, 0, sizeof(*catalog));
  memset(&scan, 0, sizeof(scan));
  scan.catalog = catalog;
  if (!sf_table_scan(
        table_path, sf_transport_number, &scan,
        sf_transport_text, &scan) ||
      catalog->count != SF_TRANSPORT_DESTINATION_COUNT) goto failed;
  for (row = 0u; row < catalog->count; ++row)
    if (scan.fields[row] != 7u || !catalog->destinations[row].name[0] ||
        catalog->destinations[row].scenario_id < 0 ||
        catalog->destinations[row].entry_value < 0) goto failed;
  return true;
failed:
  memset(catalog, 0, sizeof(*catalog));
  return false;
}

const SfTransportDestination *sf_transport_destination(
    const SfTransportCatalog *catalog, int32_t row) {
  return catalog && row >= 0 && row < catalog->count
    ? &catalog->destinations[row] : NULL;
}
