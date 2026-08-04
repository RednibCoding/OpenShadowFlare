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

#include "ui/gameplay_belt.h"

#include "ui/gameplay_inventory.h"
#include "ui/gameplay_item_icon.h"

bool sf_gameplay_belt_pocket_at(
    int16_t x, int16_t y, uint8_t *grid_x, uint8_t *grid_y) {
  if (x >= 357 && x < 485 && y >= 413 && y < 445) {
    if (grid_x) *grid_x = (uint8_t) ((x - 357) / 32);
    if (grid_y) *grid_y = 0u;
    return true;
  }
  if (x >= 405 && x < 533 && y >= 445 && y < 477) {
    if (grid_x) *grid_x = (uint8_t) ((x - 405) / 32);
    if (grid_y) *grid_y = 1u;
    return true;
  }
  return false;
}

void sf_gameplay_belt_draw(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfPlayerState *player, const SfRect *clip) {
  uint8_t index;
  if (!renderer || !assets || !player) return;
  for (index = 0u; index < player->belt.count; ++index) {
    const SfInventoryItem *item = &player->belt.items[index];
    const int x = (item->grid_y == 0u ? 357 : 405) +
      item->grid_x * SF_GAMEPLAY_INVENTORY_CELL_SIZE;
    const int y = 413 + item->grid_y * SF_GAMEPLAY_INVENTORY_CELL_SIZE;
    sf_gameplay_item_icon_draw(renderer, assets, item, x, y, clip);
  }
}
