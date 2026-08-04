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

#include "ui/gameplay_special_items.h"

#include "ui/gameplay_item_condition.h"
#include "ui/gameplay_status_pattern.h"

static bool sf_special_intersects_panel(const SfRect *clip) {
  if (!clip) return true;
  return clip->x < 320 && clip->x + clip->width > 0 &&
    clip->y < 412 && clip->y + clip->height > 0;
}

void sf_gameplay_special_items_draw(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfPlayerState *player, const SfGameplayInventoryUi *inventory,
    uint32_t gameplay_counter, const SfRect *clip) {
  uint8_t index;
  if (!renderer || !assets || !player || !inventory ||
      !inventory->special_open || !sf_special_intersects_panel(clip)) return;
  sf_renderer_fill_rect(renderer, (SfRect) {0, 0, 320, 412}, 0u);
  sf_gameplay_status_pattern_draw(
    renderer, &assets->inventory_panel, 14u, 0, 0, NULL);
  sf_gameplay_status_pattern_draw(
    renderer, &assets->inventory_panel, 15u, 0, 0, NULL);
  for (index = 0u; index < player->special_items.count; ++index) {
    const SfInventoryItem *item = &player->special_items.items[index];
    sf_gameplay_item_condition_draw(
      renderer, assets, item,
      SF_GAMEPLAY_SPECIAL_LEFT +
        item->grid_x * SF_GAMEPLAY_INVENTORY_CELL_SIZE,
      SF_GAMEPLAY_SPECIAL_TOP +
        item->grid_y * SF_GAMEPLAY_INVENTORY_CELL_SIZE,
      gameplay_counter, clip);
  }
}
