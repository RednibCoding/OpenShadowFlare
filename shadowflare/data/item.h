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

#ifndef SHADOWFLARE_DATA_ITEM_H
#define SHADOWFLARE_DATA_ITEM_H

#include <stdbool.h>
#include <stdint.h>

typedef struct SfItemAppearance {
  int32_t part;
  int32_t red;
  int32_t green;
  int32_t blue;
} SfItemAppearance;

typedef struct SfItemReference {
  int32_t definition_id;
  uint8_t category;
} SfItemReference;

typedef struct SfItemGroundDefinition {
  char name[64];
  int32_t definition_id;
  int32_t resource_id;
  int32_t animation_chart;
  int32_t red_strength;
  int32_t green_strength;
  int32_t blue_strength;
  int32_t inventory_width;
  int32_t inventory_height;
  int32_t inventory_pattern_group;
  int32_t inventory_pattern;
  int32_t inventory_palette;
  int32_t weight;
  int32_t maximum_durability;
  int32_t variant;
  uint8_t category;
} SfItemGroundDefinition;

bool sf_item_read_appearance(
  const char *path, uint8_t category, int32_t definition_id,
  SfItemAppearance *appearance);
bool sf_item_read_ground_definitions(
  const char *path, SfItemGroundDefinition *definitions,
  uint8_t definition_count);

#endif
