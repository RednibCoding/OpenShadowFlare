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

#include "ui/world_pointer_overlay.h"

bool sf_world_pointer_overlay_bounds(
    const SfWorldState *world, SfRect *bounds) {
  static const uint8_t half_sizes[5] = {0u, 12u, 16u, 24u, 48u};
  const SfWorldPointerControl *pointer;
  int half_size;
  if (!world || !bounds) return false;
  pointer = &world->pointer;
  if (!pointer->active || !pointer->range_enabled || pointer->range >= 5u ||
      pointer->screen_y >= 408) return false;
  half_size = half_sizes[pointer->range];
  if (half_size <= 0) return false;
  bounds->x = (int16_t) (pointer->screen_x - half_size);
  bounds->y = (int16_t) (pointer->screen_y - half_size);
  bounds->width = (int16_t) (half_size * 2 + 1);
  bounds->height = bounds->width;
  return true;
}

void sf_world_pointer_overlay_draw(
    SfRenderer *renderer, const SfWorldState *world) {
  const SfWorldPointerControl *pointer;
  SfRect bounds;
  SfRect line;
  int length;
  uint16_t opacity;
  if (!renderer || !world ||
      !sf_world_pointer_overlay_bounds(world, &bounds)) return;
  pointer = &world->pointer;
  length = bounds.width;
  opacity = pointer->hovered_actor_id >= 0 ||
    pointer->hovered_enemy_id >= 0 ||
    pointer->hovered_scenario_object_id >= 0 ? 300u : 100u;
  line = (SfRect) {
    bounds.x, bounds.y,
    (int16_t) length, 1};
  sf_renderer_fill_rect_blended(renderer, line, 0x7fffu, opacity);
  line.y = (int16_t) (bounds.y + length - 1);
  sf_renderer_fill_rect_blended(renderer, line, 0x7fffu, opacity);
  line.x = bounds.x;
  line.y = bounds.y;
  line.width = 1;
  line.height = (int16_t) length;
  sf_renderer_fill_rect_blended(renderer, line, 0x7fffu, opacity);
  line.x = (int16_t) (bounds.x + length - 1);
  sf_renderer_fill_rect_blended(renderer, line, 0x7fffu, opacity);
}
