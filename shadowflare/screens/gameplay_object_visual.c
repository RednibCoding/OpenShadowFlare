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

#include "screens/gameplay_object_visual.h"

#include "core/coordinates.h"
#include "core/memory_budget.h"

#include <stdint.h>

bool sf_gameplay_object_visual_find(
    const SfGameplayAssets *assets, const SfMapObject *object,
    bool shadow, SfGameplayObjectVisual *visual) {
  int set;
  if (!visual) return false;
  visual->resource = NULL;
  visual->pattern = NULL;
  if (!assets || !object) return false;
  set = object->pattern_set + (shadow ? 1 : 0);
  if (set < 0 || set > UINT8_MAX || object->pattern < 0 ||
      object->pattern > UINT8_MAX) return false;
  visual->resource = sf_gameplay_pattern_set(assets, (uint8_t) set);
  if (!visual->resource || visual->resource->is_shadow != shadow) return false;
  visual->pattern = sf_njp_decoded_pattern(
    visual->resource, (uint8_t) object->pattern);
  return visual->pattern != NULL;
}

bool sf_gameplay_object_visual_visible(
    const SfGameplayObjectVisual *visual, const SfMapObject *object,
    const SfWorldRenderView *view, bool shadow) {
  SfScreenPoint anchor;
  int64_t left;
  int64_t top;
  if (!visual || !visual->pattern || !visual->pattern->bounds.valid ||
      !object || !view) return false;
  anchor = sf_world_to_screen(
    (SfWorldPoint) {object->world_x, object->world_y});
  left = (int64_t) anchor.x - view->camera_x + visual->pattern->bounds.x;
  top = (int64_t) anchor.y - view->camera_y + visual->pattern->bounds.y;
  if (!shadow) top -= object->height * 20 / 100;
  return left < SF_FRAME_WIDTH && top < SF_FRAME_HEIGHT &&
    left + visual->pattern->bounds.width > 0 &&
    top + visual->pattern->bounds.height > 0;
}

static uint8_t sf_gameplay_object_pixel(
    const SfIndexedImage *image, uint16_t x, uint16_t y) {
  const uint16_t source_y = image->bottom_up
    ? (uint16_t) (image->height - y - 1u) : y;
  const uint8_t *row = image->pixels + (size_t) source_y * image->stride;
  if (image->bits_per_pixel == 8u) return row[x];
  if (image->bits_per_pixel == 4u) {
    const uint8_t packed = row[x >> 1u];
    return (uint8_t) ((packed >> ((x & 1u) ? 0u : 4u)) & 15u);
  }
  return (uint8_t) ((row[x >> 3u] >> (7u - (x & 7u))) & 1u);
}

static bool sf_gameplay_part_intersects(
    const SfIndexedImage *image, int left, int top, SfRect rectangle) {
  int first_x = rectangle.x > left ? rectangle.x : left;
  int first_y = rectangle.y > top ? rectangle.y : top;
  int last_x = rectangle.x + rectangle.width;
  int last_y = rectangle.y + rectangle.height;
  int y;
  if (left + image->width < last_x) last_x = left + image->width;
  if (top + image->height < last_y) last_y = top + image->height;
  for (y = first_y; y < last_y; ++y) {
    int x;
    for (x = first_x; x < last_x; ++x) {
      if (sf_gameplay_object_pixel(
            image, (uint16_t) (x - left), (uint16_t) (y - top)) != 0u)
        return true;
    }
  }
  return false;
}

bool sf_gameplay_object_visual_intersects(
    const SfGameplayObjectVisual *visual, const SfMapObject *object,
    const SfWorldRenderView *view, SfRect rectangle) {
  SfScreenPoint anchor;
  uint8_t reference;
  if (!visual || !visual->resource || !visual->pattern || !object || !view ||
      rectangle.width <= 0 || rectangle.height <= 0) return false;
  anchor = sf_world_to_screen(
    (SfWorldPoint) {object->world_x, object->world_y});
  anchor.x -= view->camera_x;
  anchor.y -= view->camera_y + object->height * 20 / 100;
  for (reference = 0u; reference < visual->pattern->reference_count;
       ++reference) {
    const SfNjpDecodedReference *item = &visual->resource->references[
      visual->pattern->first_reference + reference];
    const SfIndexedImage *image;
    if (item->part >= visual->resource->part_count) continue;
    image = &visual->resource->parts[item->part].image;
    if (sf_gameplay_part_intersects(
          image, anchor.x + item->x, anchor.y + item->y, rectangle))
      return true;
  }
  return false;
}
