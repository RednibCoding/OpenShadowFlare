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

#ifndef SHADOWFLARE_DATA_ITEM_RECORDS_H
#define SHADOWFLARE_DATA_ITEM_RECORDS_H

#include <stdbool.h>
#include <stdint.h>

typedef bool (*SfItemRecordWord)(
  void *user, uint8_t category, uint16_t offset, int32_t value);
typedef bool (*SfItemRecordText)(
  void *user, uint8_t category,
  const char *name, const char *description);

bool sf_item_scan_records(
  const char *path, SfItemRecordWord word, void *user);
bool sf_item_scan_named_records(
  const char *path, SfItemRecordText text,
  SfItemRecordWord word, void *user);

#endif
