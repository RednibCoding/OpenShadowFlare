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

#ifndef SHADOWFLARE_GAME_EQUIPMENT_H
#define SHADOWFLARE_GAME_EQUIPMENT_H

#include "game/inventory.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum SfEquipmentSlot {
  SF_EQUIPMENT_HELMET = 0,
  SF_EQUIPMENT_BODY,
  SF_EQUIPMENT_BOOTS,
  SF_EQUIPMENT_MAIN_HAND,
  SF_EQUIPMENT_OFF_HAND,
  SF_EQUIPMENT_ACCESSORY_1,
  SF_EQUIPMENT_ACCESSORY_2,
  SF_EQUIPMENT_ACCESSORY_3,
  SF_EQUIPMENT_ACCESSORY_4,
  SF_EQUIPMENT_SLOT_COUNT
} SfEquipmentSlot;

typedef struct SfEquipmentState {
  SfInventoryItem items[SF_EQUIPMENT_SLOT_COUNT];
  uint16_t occupied;
} SfEquipmentState;

typedef struct SfEquipmentPlacement {
  SfInventoryItem held_item;
  bool accepted;
  bool holding_item;
} SfEquipmentPlacement;

void sf_equipment_init(SfEquipmentState *equipment);
const SfInventoryItem *sf_equipment_item(
  const SfEquipmentState *equipment, SfEquipmentSlot slot);
const SfItemGroundDefinition *sf_equipment_find_definition(
  const SfItemGroundDefinition *definitions, uint8_t definition_count,
  const SfInventoryItem *item);
bool sf_equipment_take(
  SfEquipmentState *equipment, SfEquipmentSlot slot,
  SfInventoryItem *item);
SfEquipmentPlacement sf_equipment_place(
  SfEquipmentState *equipment, SfEquipmentSlot slot,
  SfInventoryItem item, const SfItemGroundDefinition *definition,
  int32_t player_level);
SfEquipmentSlot sf_equipment_default_slot(
  const SfItemGroundDefinition *definition);
int32_t sf_equipment_total_weight(
  const SfEquipmentState *equipment,
  const SfItemGroundDefinition *definitions, uint8_t definition_count);
const SfItemGroundDefinition *sf_equipment_part_definition(
  const SfEquipmentState *equipment,
  const SfItemGroundDefinition *definitions, uint8_t definition_count,
  uint8_t appearance_part);

#endif
