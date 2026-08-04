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

#include "ui/gameplay_equipment_layout.h"

#include "ui/gameplay_inventory.h"

static const SfRect sf_equipment_regions[SF_EQUIPMENT_SLOT_COUNT] = {
  {560, 16, 64, 64},
  {560, 88, 64, 96},
  {560, 192, 64, 64},
  {480, 16, 64, 128},
  {480, 160, 64, 96},
  {400, 143, 32, 32},
  {400, 183, 32, 32},
  {440, 143, 32, 32},
  {440, 183, 32, 32}
};

const SfRect *sf_gameplay_equipment_slot_region(SfEquipmentSlot slot) {
  if (slot < 0 || slot >= SF_EQUIPMENT_SLOT_COUNT) return NULL;
  return &sf_equipment_regions[(uint8_t) slot];
}

SfEquipmentSlot sf_gameplay_equipment_slot_at(int16_t x, int16_t y) {
  uint8_t slot;
  for (slot = 0u; slot < SF_EQUIPMENT_SLOT_COUNT; ++slot) {
    const SfRect *region = &sf_equipment_regions[slot];
    if (x >= region->x && x < region->x + region->width &&
        y >= region->y && y < region->y + region->height)
      return (SfEquipmentSlot) slot;
  }
  return SF_EQUIPMENT_SLOT_COUNT;
}

void sf_gameplay_equipment_item_origin(
    SfEquipmentSlot slot, const SfInventoryItem *item, int *x, int *y) {
  const SfRect *region = sf_gameplay_equipment_slot_region(slot);
  if (!region || !item || !x || !y) return;
  *x = region->x +
    (region->width - item->width * SF_GAMEPLAY_INVENTORY_CELL_SIZE) / 2;
  *y = region->y +
    (region->height - item->height * SF_GAMEPLAY_INVENTORY_CELL_SIZE) / 2;
}
