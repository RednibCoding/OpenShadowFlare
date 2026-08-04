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

#include "screens/gameplay_ground_item.h"

#include "core/coordinates.h"
#include "core/memory_budget.h"

static bool sf_gameplay_ground_cell_visible(
    const SfNjpSparsePattern *pattern, SfScreenPoint anchor) {
  const int32_t left = anchor.x + pattern->image.x;
  const int32_t top = anchor.y + pattern->image.y;
  return left < (int32_t) SF_FRAME_WIDTH &&
    top < (int32_t) SF_FRAME_HEIGHT &&
    left + pattern->image.image.width > 0 &&
    top + pattern->image.image.height > 0;
}

bool sf_gameplay_ground_item_visible(
    const SfGroundItemAssets *assets, const SfGroundItem *item,
    const SfWorldRenderView *view, bool shadow) {
  const SfGroundItemVisual *visual;
  const SfCafSelectedAnimation *animation;
  const SfNjpSparseResource *resource;
  SfScreenPoint anchor;
  uint8_t part;
  if (!assets || !item || !item->visible || !view) return false;
  visual = sf_ground_item_visual(assets, item->resource_id);
  animation = sf_ground_item_animation(visual, item->animation_chart);
  if (!visual || !animation || animation->frame_count == 0u) return false;
  resource = shadow ? &visual->shadows : &visual->artwork;
  anchor = sf_world_to_screen(item->position);
  anchor.x -= view->camera_x;
  anchor.y -= view->camera_y;
  if (!shadow) anchor.y -= item->height * 20 / 100;
  for (part = 0u; part < animation->part_count; ++part) {
    const SfCafCell *cell = &animation->parts[part].cells[0];
    const SfNjpSparsePattern *pattern;
    if (cell->pattern < 0 ||
        (shadow != ((cell->status & 8) != 0))) continue;
    pattern = sf_njp_sparse_pattern(resource, cell->pattern);
    if (pattern && sf_gameplay_ground_cell_visible(pattern, anchor))
      return true;
  }
  return false;
}

void sf_gameplay_ground_item_draw(
    SfRenderer *renderer, const SfGroundItemAssets *assets,
    const SfGroundItem *item, const SfWorldRenderView *view,
    bool shadow, const SfRect *clip) {
  const SfGroundItemVisual *visual;
  const SfCafSelectedAnimation *animation;
  const SfNjpSparseResource *resource;
  SfScreenPoint anchor;
  uint8_t priority;
  uint16_t red_strength;
  uint16_t green_strength;
  uint16_t blue_strength;
  if (!renderer || !assets || !item || !item->visible || !view) return;
  visual = sf_ground_item_visual(assets, item->resource_id);
  animation = sf_ground_item_animation(visual, item->animation_chart);
  if (!visual || !animation || animation->frame_count == 0u) return;
  resource = shadow ? &visual->shadows : &visual->artwork;
  anchor = sf_world_to_screen(item->position);
  anchor.x -= view->camera_x;
  anchor.y -= view->camera_y;
  if (!shadow) anchor.y -= item->height * 20 / 100;
  red_strength = item->red_strength < 0 ? 0u :
    item->red_strength > 2000 ? 2000u : (uint16_t) item->red_strength;
  green_strength = item->green_strength < 0 ? 0u :
    item->green_strength > 2000 ? 2000u : (uint16_t) item->green_strength;
  blue_strength = item->blue_strength < 0 ? 0u :
    item->blue_strength > 2000 ? 2000u : (uint16_t) item->blue_strength;
  for (priority = animation->priority_count; priority > 0u; --priority) {
    uint8_t part;
    for (part = 0u; part < animation->part_count; ++part) {
      const SfCafCell *cell = &animation->parts[part].cells[0];
      const SfNjpSparsePattern *pattern;
      SfIndexedImage image;
      uint16_t opacity;
      SfBlendMode blend;
      if (cell->priority != (int16_t) (priority - 1u) ||
          cell->pattern < 0 ||
          (shadow != ((cell->status & 8) != 0))) continue;
      pattern = sf_njp_sparse_pattern(resource, cell->pattern);
      if (!pattern) continue;
      image = pattern->image.image;
      if (!shadow && animation->palette_mode == 1) {
        const int32_t palette_source =
          animation->chart_priority_stride * item->animation_chart +
          cell->priority;
        const uint16_t *palette = sf_njp_sparse_palette(
          resource, palette_source);
        if (palette) image.palette = palette;
      }
      opacity = shadow ? 500u : cell->transparency < 0 ? 0u :
        cell->transparency > 1000 ? 1000u :
        (uint16_t) cell->transparency;
      blend = shadow ? SF_BLEND_TRANSLUCENT :
        (cell->status & 0x10) != 0 ? SF_BLEND_ADDITIVE : SF_BLEND_MASKED;
      sf_renderer_draw_indexed_tinted(
        renderer, &image,
        anchor.x + pattern->image.x, anchor.y + pattern->image.y,
        shadow ? 1000u : red_strength,
        shadow ? 1000u : green_strength,
        shadow ? 1000u : blue_strength,
        opacity, blend, clip);
    }
  }
}
