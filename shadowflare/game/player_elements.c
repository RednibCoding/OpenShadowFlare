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

#include "game/player_elements.h"

#include "game/equipment.h"

#include <limits.h>

typedef struct SfElementAnchor {
  int32_t x;
  int32_t y;
} SfElementAnchor;

static uint32_t sf_element_square_root(uint64_t value) {
  uint64_t root = 0u;
  uint64_t bit = (uint64_t) 1u << 62u;
  while (bit > value) bit >>= 2u;
  while (bit != 0u) {
    if (value >= root + bit) {
      value -= root + bit;
      root = (root >> 1u) + bit;
    } else {
      root >>= 1u;
    }
    bit >>= 2u;
  }
  return root > UINT32_MAX ? UINT32_MAX : (uint32_t) root;
}

static void sf_element_add_item(
    int32_t affinities[SF_PLAYER_ELEMENT_COUNT],
    const SfInventoryItem *item,
    const SfItemGroundDefinition *definitions, uint8_t definition_count) {
  const SfItemGroundDefinition *definition;
  uint8_t element;
  if (!item || (item->category <= 1u && item->durability == 0)) return;
  definition = sf_equipment_find_definition(
    definitions, definition_count, item);
  if (!definition) return;
  for (element = 0u; element < SF_PLAYER_ELEMENT_COUNT; ++element)
    affinities[element] += definition->element_strengths[element];
}

void sf_player_element_affinities(
    const SfPlayerState *player,
    const SfItemGroundDefinition *definitions, uint8_t definition_count,
    int8_t affinities[SF_PLAYER_ELEMENT_COUNT]) {
  static const SfElementAnchor anchors[SF_PLAYER_ELEMENT_COUNT] = {
    {0, 20000}, {0, -20000}, {-20000, 0}, {20000, 0},
    {14140, -14140}, {-14140, 14140},
    {-14140, -14140}, {14140, 14140}
  };
  int32_t values[SF_PLAYER_ELEMENT_COUNT];
  uint8_t element;
  uint8_t slot;
  if (!player || !affinities) return;
  for (element = 0u; element < SF_PLAYER_ELEMENT_COUNT; ++element) {
    const int64_t dx = (int64_t) anchors[element].x - player->element_x;
    const int64_t dy = (int64_t) anchors[element].y - player->element_y;
    const uint32_t distance = sf_element_square_root(
      (uint64_t) (dx * dx + dy * dy));
    values[element] = (20000 - (int32_t) distance) / 2000;
  }
  for (slot = 0u; slot < SF_EQUIPMENT_VISIBLE_SLOT_COUNT; ++slot) {
    const SfInventoryItem *item = sf_equipment_item(
      &player->equipment, (SfEquipmentSlot) slot);
    if (slot == SF_EQUIPMENT_OFF_HAND) {
      const SfInventoryItem *main_hand = sf_equipment_item(
        &player->equipment, SF_EQUIPMENT_MAIN_HAND);
      const SfItemGroundDefinition *main_definition = main_hand
        ? sf_equipment_find_definition(
            definitions, definition_count, main_hand) : NULL;
      if (main_definition && main_definition->suppresses_off_hand) continue;
    }
    sf_element_add_item(
      values, item, definitions, definition_count);
  }
  for (slot = 0u; slot < player->inventory.count; ++slot) {
    const SfInventoryItem *item = &player->inventory.items[slot];
    const SfItemGroundDefinition *definition;
    if (item->category != 2u || !item->identified) continue;
    definition = sf_equipment_find_definition(
      definitions, definition_count, item);
    if (!definition || definition->inventory_width == 1) continue;
    sf_element_add_item(values, item, definitions, definition_count);
  }
  for (element = 0u; element < SF_PLAYER_ELEMENT_COUNT; ++element) {
    if (values[element] < -10) values[element] = -10;
    if (values[element] > 10) values[element] = 10;
    affinities[element] = (int8_t) values[element];
  }
}
