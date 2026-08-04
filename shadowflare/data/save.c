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

#include "data/save.h"

#include "assets/retail_paths.h"

#include <stdio.h>
#include <string.h>

static int32_t sf_save_i32(const uint8_t *bytes) {
  const uint32_t value = (uint32_t) bytes[0] |
    ((uint32_t) bytes[1] << 8u) | ((uint32_t) bytes[2] << 16u) |
    ((uint32_t) bytes[3] << 24u);
  return (int32_t) value;
}

static bool sf_save_path(
    char *path, size_t capacity, const char *data_root,
    const char *format, uint8_t file_slot) {
  char relative[32];
  const int length = snprintf(relative, sizeof(relative), format, file_slot);
  return length >= 0 && (size_t) length < sizeof(relative) &&
    sf_retail_path_join(path, capacity, data_root, relative);
}

bool sf_save_slot_data_path(
    char *path, size_t capacity, const char *data_root, uint8_t file_slot) {
  return sf_save_path(
    path, capacity, data_root,
    sf_retail_save_paths.data_format, file_slot);
}

bool sf_save_catalog_load(const char *data_root, SfSaveCatalog *catalog) {
  static const uint8_t signature[16] = {
    'S', 'h', 'a', 'd', 'o', 'w', 'F', 'l',
    'a', 'r', 'e', '0', '0', '0', '5', 0
  };
  uint8_t file_slot;
  if (!data_root || !catalog) return false;
  memset(catalog, 0, sizeof(*catalog));
  for (file_slot = 0u; file_slot < SF_SAVE_SLOT_COUNT; ++file_slot) {
    uint8_t header[16];
    uint8_t bytes[0x160];
    SfSaveSummary *summary;
    char path[SF_RETAIL_PATH_CAPACITY];
    FILE *file;
    if (!sf_save_slot_data_path(
          path, sizeof(path), data_root, file_slot)) return false;
    file = fopen(path, "rb");
    if (!file) continue;
    if (fread(header, 1u, sizeof(header), file) != sizeof(header) ||
        fread(bytes, 1u, sizeof(bytes), file) != sizeof(bytes)) {
      fclose(file);
      continue;
    }
    fclose(file);
    if (memcmp(header, signature, sizeof(signature)) != 0) continue;
    summary = &catalog->entries[catalog->count++];
    memcpy(summary->name, bytes, 16u);
    summary->name[16] = '\0';
    summary->gender = sf_save_i32(bytes + 0x18u);
    summary->job = sf_save_i32(bytes + 0x1cu);
    summary->level = sf_save_i32(bytes + 0x24u);
    summary->life = sf_save_i32(bytes + 0x30u);
    summary->mana = sf_save_i32(bytes + 0x38u);
    summary->experience = sf_save_i32(bytes + 0xd8u);
    summary->file_slot = file_slot;
  }
  return true;
}

bool sf_save_catalog_delete(
    const char *data_root, const SfSaveCatalog *catalog,
    uint8_t catalog_index) {
  char path[SF_RETAIL_PATH_CAPACITY];
  uint8_t file_slot;
  if (!data_root || !catalog || catalog_index >= catalog->count) return false;
  file_slot = catalog->entries[catalog_index].file_slot;
  if (!sf_save_slot_data_path(
        path, sizeof(path), data_root, file_slot) || remove(path) != 0)
    return false;
  if (sf_save_path(
        path, sizeof(path), data_root,
        sf_retail_save_paths.preview_format, file_slot)) (void) remove(path);
  return true;
}
