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

#include "ui/ground_item_nameplate.h"

#include "core/coordinates.h"

#include <stdio.h>
#include <string.h>

static const SfGroundItem *sf_ground_item_nameplate_item(
    const SfWorldState *world) {
  uint8_t index;
  if (!world || world->pointer.hovered_ground_item_id < 0) return NULL;
  for (index = 0u; index < world->ground_items.count; ++index)
    if (world->ground_items.items[index].id ==
        world->pointer.hovered_ground_item_id)
      return &world->ground_items.items[index];
  return NULL;
}

static bool sf_ground_item_nameplate_text(
    const SfWorldState *world, const SfGroundItem *item,
    char *text, size_t capacity) {
  const SfItemGroundDefinition *definition;
  if (!world || !item || !text || capacity == 0u) return false;
  definition = sf_ground_items_definition(
    &world->ground_items, item->category, item->definition_id);
  if (!definition) return false;
  if (item->category == 4u && item->definition_id == 0)
    return snprintf(text, capacity, "%d Gold", (int) item->quantity) > 0;
  if (!definition->name[0] || strlen(definition->name) >= capacity)
    return false;
  memcpy(text, definition->name, strlen(definition->name) + 1u);
  return true;
}

bool sf_ground_item_nameplate_bounds(
    const SfGameplayAssets *assets, const SfWorldState *world,
    const SfWorldRenderView *view, SfRect *bounds) {
  const SfGroundItem *item = sf_ground_item_nameplate_item(world);
  const SfIndexedImage *font;
  SfScreenPoint anchor;
  char text[80];
  int cell_width;
  int half_width;
  if (!assets || !view || !bounds || !item ||
      assets->font.image_count == 0u ||
      !sf_ground_item_nameplate_text(
        world, item, text, sizeof(text))) return false;
  font = &assets->font.images[0].image;
  cell_width = font->width / 16u;
  if (cell_width <= 0) return false;
  anchor = sf_world_to_screen(item->position);
  anchor.x -= view->camera_x;
  anchor.y -= view->camera_y;
  half_width = (int) strlen(text) * cell_width / 2;
  bounds->x = (int16_t) (anchor.x + 2 - half_width - 4);
  bounds->y = (int16_t) (anchor.y - 26);
  bounds->width = (int16_t) (half_width * 2 + 5);
  bounds->height = 15;
  return true;
}

void sf_ground_item_nameplate_draw(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfWorldState *world, const SfWorldRenderView *view) {
  const SfGroundItem *item = sf_ground_item_nameplate_item(world);
  SfRect background;
  char text[80];
  if (!renderer || !assets || !item ||
      !sf_ground_item_nameplate_bounds(assets, world, view, &background) ||
      !sf_ground_item_nameplate_text(
        world, item, text, sizeof(text))) return;
  sf_renderer_fill_rect_blended(renderer, background, 0u, 500u);
  sf_renderer_draw_text(
    renderer, &assets->font.images[0].image, text,
    background.x + 5, background.y + 3, 0u, 1000u);
  sf_renderer_draw_text(
    renderer, &assets->font.images[0].image, text,
    background.x + 4, background.y + 2, sf_rgb555(28u, 28u, 28u), 1000u);
}
