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

#include "game/item_condition.h"

static int32_t sf_item_condition_durability(
    const SfInventoryItem *item,
    const SfItemGroundDefinition *definition) {
  int32_t durability = item->durability >= 0
    ? item->durability : definition->maximum_durability;
  if (durability < 0) durability = 0;
  if (durability > definition->maximum_durability)
    durability = definition->maximum_durability;
  return durability;
}

static bool sf_item_condition_low(
    const SfInventoryItem *item,
    const SfItemGroundDefinition *definition,
    int32_t *durability) {
  if (!item || !definition || item->category > 1u ||
      item->category != definition->category ||
      item->definition_id != definition->definition_id ||
      definition->maximum_durability <= 0) return false;
  *durability = sf_item_condition_durability(item, definition);
  return (int64_t) *durability * 100 / definition->maximum_durability <= 9;
}

bool sf_item_condition_warning_visible(
    const SfInventoryItem *item,
    const SfItemGroundDefinition *definition,
    uint32_t gameplay_counter) {
  int32_t durability;
  if (!sf_item_condition_low(item, definition, &durability)) return false;
  return durability == 0 || (gameplay_counter & 15u) <= 7u;
}

bool sf_item_condition_warning_blinks(
    const SfInventoryItem *item,
    const SfItemGroundDefinition *definition) {
  int32_t durability;
  return sf_item_condition_low(item, definition, &durability) &&
    durability > 0;
}
