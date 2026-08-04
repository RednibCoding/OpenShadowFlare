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

#include <string.h>

#define SF_ITEM_GROUND_DEFINITION_LIMIT 16u

typedef struct SfItemGroundScan {
  SfItemGroundDefinition *definitions;
  uint16_t found;
  int8_t active;
  uint8_t count;
  char record_name[64];
} SfItemGroundScan;

static bool sf_item_ground_name(
    void *user, uint8_t category, const char *name) {
  SfItemGroundScan *scan = (SfItemGroundScan *) user;
  (void) category;
  memcpy(scan->record_name, name, sizeof(scan->record_name));
  scan->record_name[sizeof(scan->record_name) - 1u] = '\0';
  return true;
}

static int sf_item_ground_find(
    const SfItemGroundScan *scan, uint8_t category, int32_t id) {
  uint8_t index;
  for (index = 0u; index < scan->count; ++index) {
    const SfItemGroundDefinition *definition = &scan->definitions[index];
    if (definition->category == category && definition->definition_id == id)
      return index;
  }
  return -1;
}

static bool sf_item_ground_word(
    void *user, uint8_t category, uint16_t offset, int32_t value) {
  SfItemGroundScan *scan = (SfItemGroundScan *) user;
  SfItemGroundDefinition *definition;
  if (offset == 4u) {
    scan->active = (int8_t) sf_item_ground_find(scan, category, value);
    if (scan->active >= 0) {
      definition = &scan->definitions[(uint8_t) scan->active];
      memcpy(definition->name, scan->record_name, sizeof(definition->name));
      definition->name[sizeof(definition->name) - 1u] = '\0';
      scan->found = (uint16_t) (
        scan->found | (uint16_t) (1u << (uint8_t) scan->active));
    }
  }
  if (scan->active < 0) return true;
  definition = &scan->definitions[(uint8_t) scan->active];
  if (offset == 8u) definition->variant = value;
  if (offset == 28u) definition->inventory_width = value;
  if (offset == 32u) definition->inventory_height = value;
  if (offset == 36u) definition->weight = value;
  if (offset == 48u) definition->resource_id = value;
  if (offset == 52u) definition->animation_chart = value;
  if (offset == 60u) definition->red_strength = value;
  if (offset == 64u) definition->green_strength = value;
  if (offset == 68u) definition->blue_strength = value;
  if (offset == 100u && category <= 1u)
    definition->maximum_durability = value;
  return true;
}

bool sf_item_read_ground_definitions(
    const char *path, SfItemGroundDefinition *definitions,
    uint8_t definition_count) {
  SfItemGroundScan scan;
  uint8_t index;
  uint16_t all;
  if (!path || !definitions || definition_count == 0u ||
      definition_count > SF_ITEM_GROUND_DEFINITION_LIMIT) return false;
  for (index = 0u; index < definition_count; ++index) {
    if (definitions[index].category >= 5u) return false;
    definitions[index].resource_id = -1;
    definitions[index].animation_chart = -1;
    definitions[index].red_strength = 1000;
    definitions[index].green_strength = 1000;
    definitions[index].blue_strength = 1000;
    definitions[index].inventory_width = 1;
    definitions[index].inventory_height = 1;
    definitions[index].maximum_durability = 0;
    definitions[index].name[0] = '\0';
  }
  scan = (SfItemGroundScan) {
    definitions, 0u, -1, definition_count, {0}};
  all = (uint16_t) ((1u << definition_count) - 1u);
  return sf_item_scan_named_records(
      path, sf_item_ground_name, sf_item_ground_word, &scan) &&
    scan.found == all;
}
