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

#include "game/equipment.h"

#include <limits.h>
#include <string.h>

void sf_equipment_init(SfEquipmentState *equipment) {
  if (equipment) memset(equipment, 0, sizeof(*equipment));
}

const SfInventoryItem *sf_equipment_item(
    const SfEquipmentState *equipment, SfEquipmentSlot slot) {
  if (!equipment || slot < 0 || slot >= SF_EQUIPMENT_SLOT_COUNT ||
      (equipment->occupied & (uint16_t) (1u << (unsigned) slot)) == 0u)
    return NULL;
  return &equipment->items[(uint8_t) slot];
}

bool sf_equipment_take(
    SfEquipmentState *equipment, SfEquipmentSlot slot,
    SfInventoryItem *item) {
  const uint16_t mask = slot >= 0 && slot < SF_EQUIPMENT_SLOT_COUNT
    ? (uint16_t) (1u << (unsigned) slot) : 0u;
  if (!equipment || !item || mask == 0u ||
      (equipment->occupied & mask) == 0u) return false;
  *item = equipment->items[(uint8_t) slot];
  equipment->occupied = (uint16_t) (equipment->occupied & ~mask);
  return true;
}

static bool sf_equipment_accepts(
    SfEquipmentSlot slot, const SfItemGroundDefinition *definition) {
  if (!definition) return false;
  if (slot == SF_EQUIPMENT_MAIN_HAND) return definition->category == 0u;
  if (slot >= SF_EQUIPMENT_ACCESSORY_1 &&
      slot <= SF_EQUIPMENT_ACCESSORY_4)
    return definition->category == 2u && definition->inventory_width == 1;
  if (definition->category != 1u) return false;
  if (slot == SF_EQUIPMENT_HELMET) return definition->subtype == 0;
  if (slot == SF_EQUIPMENT_BODY) return definition->subtype == 1;
  if (slot == SF_EQUIPMENT_BOOTS) return definition->subtype == 3;
  return slot == SF_EQUIPMENT_OFF_HAND && definition->subtype == 2;
}

SfEquipmentPlacement sf_equipment_place(
    SfEquipmentState *equipment, SfEquipmentSlot slot,
    SfInventoryItem item, const SfItemGroundDefinition *definition,
    int32_t player_level) {
  SfEquipmentPlacement result;
  const uint16_t mask = slot >= 0 && slot < SF_EQUIPMENT_SLOT_COUNT
    ? (uint16_t) (1u << (unsigned) slot) : 0u;
  memset(&result, 0, sizeof(result));
  if (!equipment || mask == 0u || !sf_equipment_accepts(slot, definition) ||
      item.category != definition->category ||
      item.definition_id != definition->definition_id ||
      item.quantity != 1 || player_level < definition->required_level)
    return result;
  if ((equipment->occupied & mask) != 0u) {
    result.held_item = equipment->items[(uint8_t) slot];
    result.holding_item = true;
  }
  item.grid_x = 0u;
  item.grid_y = 0u;
  equipment->items[(uint8_t) slot] = item;
  equipment->occupied = (uint16_t) (equipment->occupied | mask);
  result.accepted = true;
  return result;
}

SfEquipmentSlot sf_equipment_default_slot(
    const SfItemGroundDefinition *definition) {
  if (!definition) return SF_EQUIPMENT_SLOT_COUNT;
  if (definition->category == 0u) return SF_EQUIPMENT_MAIN_HAND;
  if (definition->category == 1u && definition->subtype == 0)
    return SF_EQUIPMENT_HELMET;
  if (definition->category == 1u && definition->subtype == 1)
    return SF_EQUIPMENT_BODY;
  if (definition->category == 1u && definition->subtype == 2)
    return SF_EQUIPMENT_OFF_HAND;
  if (definition->category == 1u && definition->subtype == 3)
    return SF_EQUIPMENT_BOOTS;
  if (definition->category == 2u && definition->inventory_width == 1)
    return SF_EQUIPMENT_ACCESSORY_1;
  return SF_EQUIPMENT_SLOT_COUNT;
}

const SfItemGroundDefinition *sf_equipment_find_definition(
    const SfItemGroundDefinition *definitions, uint8_t count,
    const SfInventoryItem *item) {
  uint8_t index;
  if (!definitions || !item) return NULL;
  for (index = 0u; index < count; ++index)
    if (definitions[index].category == item->category &&
        definitions[index].definition_id == item->definition_id)
      return &definitions[index];
  return NULL;
}

static bool sf_equipment_off_hand_hidden(
    const SfEquipmentState *equipment,
    const SfItemGroundDefinition *definitions, uint8_t definition_count) {
  const SfInventoryItem *main_hand = sf_equipment_item(
    equipment, SF_EQUIPMENT_MAIN_HAND);
  const SfItemGroundDefinition *definition = main_hand
    ? sf_equipment_find_definition(
        definitions, definition_count, main_hand) : NULL;
  return definition && definition->suppresses_off_hand;
}

int32_t sf_equipment_total_weight(
    const SfEquipmentState *equipment,
    const SfItemGroundDefinition *definitions, uint8_t definition_count) {
  int32_t total = 0;
  uint8_t slot;
  if (!equipment || !definitions) return 0;
  for (slot = 0u; slot < SF_EQUIPMENT_SLOT_COUNT; ++slot) {
    const SfInventoryItem *item = sf_equipment_item(
      equipment, (SfEquipmentSlot) slot);
    const SfItemGroundDefinition *definition = item
      ? sf_equipment_find_definition(
          definitions, definition_count, item) : NULL;
    int64_t weight;
    if (slot == SF_EQUIPMENT_OFF_HAND && sf_equipment_off_hand_hidden(
          equipment, definitions, definition_count)) continue;
    if (!definition) continue;
    weight = (int64_t) definition->weight * item->quantity + total;
    total = weight > INT32_MAX ? INT32_MAX :
      weight < INT32_MIN ? INT32_MIN : (int32_t) weight;
  }
  return total;
}

const SfItemGroundDefinition *sf_equipment_part_definition(
    const SfEquipmentState *equipment,
    const SfItemGroundDefinition *definitions, uint8_t definition_count,
    uint8_t appearance_part) {
  uint8_t slot;
  if (!equipment || !definitions) return NULL;
  for (slot = 0u; slot < SF_EQUIPMENT_SLOT_COUNT; ++slot) {
    const SfInventoryItem *item;
    const SfItemGroundDefinition *definition;
    if (slot == SF_EQUIPMENT_OFF_HAND && sf_equipment_off_hand_hidden(
          equipment, definitions, definition_count)) continue;
    item = sf_equipment_item(equipment, (SfEquipmentSlot) slot);
    definition = item ? sf_equipment_find_definition(
      definitions, definition_count, item) : NULL;
    if (definition && definition->appearance_part == appearance_part)
      return definition;
  }
  return NULL;
}
