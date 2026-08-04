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

#include "ui/gameplay_item_icon.h"

const SfItemGroundDefinition *sf_gameplay_item_definition(
    const SfGameplayAssets *assets, const SfInventoryItem *item) {
  uint8_t index;
  if (!assets || !item) return NULL;
  for (index = 0u; index < assets->ground_items.definition_count; ++index) {
    const SfItemGroundDefinition *definition =
      &assets->ground_items.definitions[index];
    if (definition->category == item->category &&
        definition->definition_id == item->definition_id) return definition;
  }
  return NULL;
}

void sf_gameplay_item_icon_draw(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfInventoryItem *item, int x, int y, const SfRect *clip) {
  const SfItemGroundDefinition *definition =
    sf_gameplay_item_definition(assets, item);
  const SfNjpSparseResource *resource;
  const SfNjpSparsePattern *pattern;
  SfIndexedImage image;
  if (!renderer || !definition || definition->inventory_pattern_group < 0 ||
      definition->inventory_pattern < 0) return;
  resource = sf_inventory_item_artwork(
    &assets->inventory_items, definition->inventory_pattern_group);
  pattern = sf_njp_sparse_pattern(resource, definition->inventory_pattern);
  if (!pattern) return;
  image = pattern->image.image;
  if (definition->inventory_palette >= 0) {
    const uint16_t *palette = sf_njp_sparse_palette(
      resource, definition->inventory_palette);
    if (!palette) return;
    image.palette = palette;
  }
  sf_renderer_draw_indexed(
    renderer, &image, x + pattern->image.x, y + pattern->image.y,
    1000u, 1000u, SF_BLEND_MASKED, clip);
}
