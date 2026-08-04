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

#ifndef SHADOWFLARE_DATA_TRANSPORT_H
#define SHADOWFLARE_DATA_TRANSPORT_H

#include <stdbool.h>
#include <stdint.h>

#define SF_TRANSPORT_DESTINATION_COUNT 51u
#define SF_TRANSPORT_NAME_CAPACITY 48u

typedef struct SfTransportDestination {
  char name[SF_TRANSPORT_NAME_CAPACITY];
  int32_t scenario_id;
  int32_t entry_value;
} SfTransportDestination;

typedef struct SfTransportCatalog {
  SfTransportDestination destinations[SF_TRANSPORT_DESTINATION_COUNT];
  uint8_t count;
} SfTransportCatalog;

bool sf_transport_catalog_load(
  const char *table_path, SfTransportCatalog *catalog);
const SfTransportDestination *sf_transport_destination(
  const SfTransportCatalog *catalog, int32_t row);

#endif
