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

#ifndef SHADOWFLARE_UI_GAMEPLAY_INVENTORY_H
#define SHADOWFLARE_UI_GAMEPLAY_INVENTORY_H

#include "assets/gameplay_assets.h"
#include "game/player.h"
#include "render/renderer.h"

#include <stdbool.h>

#define SF_GAMEPLAY_INVENTORY_PANEL_LEFT 320
#define SF_GAMEPLAY_INVENTORY_BACKPACK_LEFT 336
#define SF_GAMEPLAY_INVENTORY_BACKPACK_TOP 264
#define SF_GAMEPLAY_INVENTORY_CELL_SIZE 32
#define SF_GAMEPLAY_INVENTORY_VIEW_OFFSET 160
#define SF_GAMEPLAY_SPECIAL_LEFT 16
#define SF_GAMEPLAY_SPECIAL_TOP 72

typedef struct SfGameplayInventoryUi {
  int16_t pointer_x;
  int16_t pointer_y;
  int8_t hovered_item_index;
  int8_t hovered_equipment_slot;
  int8_t hovered_special_item_index;
  uint8_t item_hover_updates;
  bool open;
  bool special_open;
  bool close_hovered;
} SfGameplayInventoryUi;

void sf_gameplay_inventory_init(SfGameplayInventoryUi *inventory);
void sf_gameplay_inventory_draw(
  SfRenderer *renderer, const SfGameplayAssets *assets,
  const SfPlayerState *player, const SfGameplayInventoryUi *inventory,
  uint32_t gameplay_counter, const SfRect *clip);
void sf_gameplay_inventory_draw_held(
  SfRenderer *renderer, const SfGameplayAssets *assets,
  const SfPlayerState *player, const SfGameplayInventoryUi *inventory,
  uint32_t gameplay_counter);

#endif
