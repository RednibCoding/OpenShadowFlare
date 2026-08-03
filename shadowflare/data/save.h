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

#ifndef SHADOWFLARE_DATA_SAVE_H
#define SHADOWFLARE_DATA_SAVE_H

#include <stdbool.h>
#include <stdint.h>

#define SF_SAVE_SLOT_COUNT 6u

typedef struct SfSaveSummary {
  char name[17];
  int32_t gender;
  int32_t job;
  int32_t level;
  int32_t life;
  int32_t mana;
  int32_t experience;
  uint8_t file_slot;
} SfSaveSummary;

typedef struct SfSaveCatalog {
  SfSaveSummary entries[SF_SAVE_SLOT_COUNT];
  uint8_t count;
} SfSaveCatalog;

bool sf_save_catalog_load(const char *data_root, SfSaveCatalog *catalog);
bool sf_save_catalog_delete(
  const char *data_root, const SfSaveCatalog *catalog, uint8_t catalog_index);

#endif
