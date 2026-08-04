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

#include "ui/scenario_object_nameplate.h"

#include "core/coordinates.h"

#include <string.h>

static const SfScenarioObject *sf_scenario_object_nameplate_object(
    const SfWorldState *world) {
  uint8_t index;
  if (!world || world->pointer.hovered_scenario_object_id < 0) return NULL;
  for (index = 0u; index < world->scenario_objects.count; ++index)
    if (world->scenario_objects.objects[index].id ==
        world->pointer.hovered_scenario_object_id)
      return &world->scenario_objects.objects[index];
  return NULL;
}

static const SfMctObject *sf_scenario_object_nameplate_record(
    const SfGameplayAssets *assets, int32_t object_id) {
  uint8_t index;
  if (!assets) return NULL;
  for (index = 0u; index < assets->scenario.object_count; ++index)
    if (assets->scenario.objects[index].id == object_id)
      return &assets->scenario.objects[index];
  return NULL;
}

bool sf_scenario_object_nameplate_bounds(
    const SfGameplayAssets *assets, const SfWorldState *world,
    const SfWorldRenderView *view, SfRect *bounds) {
  const SfScenarioObject *object = sf_scenario_object_nameplate_object(world);
  const SfMctObject *record;
  const SfIndexedImage *font;
  SfScreenPoint anchor;
  int cell_width;
  int half_width;
  if (!assets || !view || !bounds || !object) return false;
  record = sf_scenario_object_nameplate_record(assets, object->id);
  if (!record || !record->name[0] || assets->font.image_count == 0u)
    return false;
  font = &assets->font.images[0].image;
  cell_width = font->width / 16u;
  if (cell_width <= 0) return false;
  anchor = sf_world_to_screen(object->position);
  anchor.x -= view->camera_x;
  anchor.y -= view->camera_y;
  half_width = (int) strlen(record->name) * cell_width / 2;
  bounds->x = (int16_t) (anchor.x - half_width - 4);
  bounds->y = (int16_t) (anchor.y - record->label_height - 2);
  bounds->width = (int16_t) (half_width * 2 + 5);
  bounds->height = 15;
  return true;
}

void sf_scenario_object_nameplate_draw(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfWorldState *world, const SfWorldRenderView *view) {
  const SfScenarioObject *object = sf_scenario_object_nameplate_object(world);
  const SfMctObject *record;
  const SfIndexedImage *font;
  SfRect background;
  uint16_t color;
  if (!renderer || !assets || !view || !object ||
      !sf_scenario_object_nameplate_bounds(
        assets, world, view, &background)) return;
  record = sf_scenario_object_nameplate_record(assets, object->id);
  if (!record) return;
  font = &assets->font.images[0].image;
  color = sf_rgb555(
    (uint8_t) ((record->name_color & 0xffu) >> 3u),
    (uint8_t) (((record->name_color >> 8u) & 0xffu) >> 3u),
    (uint8_t) (((record->name_color >> 16u) & 0xffu) >> 3u));
  sf_renderer_fill_rect_blended(renderer, background, 0u, 500u);
  sf_renderer_draw_text(
    renderer, font, record->name,
    background.x + 5, background.y + 3, 0u, 1000u);
  sf_renderer_draw_text(
    renderer, font, record->name,
    background.x + 4, background.y + 2, color, 1000u);
}
