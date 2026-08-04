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

#include "ui/gameplay_item_condition.h"

#include "game/item_condition.h"
#include "ui/gameplay_item_icon.h"
#include "ui/gameplay_status_pattern.h"

void sf_gameplay_item_condition_draw(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfInventoryItem *item, int x, int y,
    uint32_t gameplay_counter, const SfRect *clip) {
  const SfItemGroundDefinition *definition =
    sf_gameplay_item_definition(assets, item);
  sf_gameplay_item_icon_draw(renderer, assets, item, x, y, clip);
  if (!definition || !sf_item_condition_warning_visible(
        item, definition, gameplay_counter)) return;
  sf_gameplay_status_pattern_draw(
    renderer, &assets->inventory_panel, 16u,
    x + item->width * SF_GAMEPLAY_INVENTORY_CELL_SIZE - 16,
    y + item->height * SF_GAMEPLAY_INVENTORY_CELL_SIZE - 16, clip);
}

static bool sf_gameplay_item_blinks(
    const SfGameplayAssets *assets, const SfInventoryItem *item) {
  return sf_item_condition_warning_blinks(
    item, sf_gameplay_item_definition(assets, item));
}

bool sf_gameplay_item_condition_animation_active(
    const SfGameplayAssets *assets, const SfPlayerState *player,
    const SfGameplayInventoryUi *inventory) {
  uint8_t index;
  if (!assets || !player || !inventory) return false;
  if (player->inventory_transfer.holding_item && sf_gameplay_item_blinks(
        assets, &player->inventory_transfer.held_item)) return true;
  if (!inventory->open) return false;
  for (index = 0u; index < SF_EQUIPMENT_VISIBLE_SLOT_COUNT; ++index) {
    const SfInventoryItem *item = sf_equipment_item(
      &player->equipment, (SfEquipmentSlot) index);
    if (item && sf_gameplay_item_blinks(assets, item)) return true;
  }
  for (index = 0u; index < player->inventory.count; ++index)
    if (sf_gameplay_item_blinks(assets, &player->inventory.items[index]))
      return true;
  return false;
}
