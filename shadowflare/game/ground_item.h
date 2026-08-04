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

#ifndef SHADOWFLARE_GAME_GROUND_ITEM_H
#define SHADOWFLARE_GAME_GROUND_ITEM_H

#include "core/coordinates.h"
#include "data/item.h"
#include "data/obl.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_GROUND_ITEM_LIMIT 64u

typedef struct SfGroundItem {
  SfWorldPoint position;
  SfObjectBounds judgement;
  int32_t definition_id;
  int32_t resource_id;
  int32_t animation_chart;
  int32_t quantity;
  int32_t durability;
  int32_t height;
  int32_t vertical_velocity;
  int32_t vertical_gravity;
  int32_t red_strength;
  int32_t green_strength;
  int32_t blue_strength;
  int32_t id;
  uint8_t category;
  uint8_t bounce_state;
  bool identified;
  bool visible;
} SfGroundItem;

typedef struct SfGroundItemSet {
  SfGroundItem items[SF_GROUND_ITEM_LIMIT];
  const SfItemGroundDefinition *definitions;
  uint32_t random_state;
  uint32_t presentation_revision;
  int32_t next_id;
  uint8_t definition_count;
  uint8_t count;
  uint16_t sound_samples[8];
  uint8_t sound_count;
} SfGroundItemSet;

void sf_ground_items_init(SfGroundItemSet *items);
void sf_ground_items_change_scenario(SfGroundItemSet *items);
void sf_ground_items_bind_definitions(
  SfGroundItemSet *items, const SfItemGroundDefinition *definitions,
  uint8_t definition_count);
bool sf_ground_items_create(
  SfGroundItemSet *items, int32_t category, int32_t definition_id,
  SfWorldPoint position, int32_t minimum_quantity,
  int32_t maximum_quantity);
bool sf_ground_items_create_instance(
  SfGroundItemSet *items, uint8_t category, int32_t definition_id,
  int32_t quantity, int32_t durability, bool identified,
  SfWorldPoint position);
void sf_ground_items_update(SfGroundItemSet *items);
const SfItemGroundDefinition *sf_ground_items_definition(
  const SfGroundItemSet *items, uint8_t category, int32_t definition_id);
SfGroundItem *sf_ground_items_find(SfGroundItemSet *items, int32_t id);
void sf_ground_item_restart_drop(SfGroundItem *item);
bool sf_ground_items_remove(SfGroundItemSet *items, int32_t id);
void sf_ground_items_emit_sound(SfGroundItemSet *items, uint16_t sample);

#endif
