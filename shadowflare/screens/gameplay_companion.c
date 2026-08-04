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

#include "screens/gameplay_companion.h"

#include "core/coordinates.h"
#include "core/memory_budget.h"

static bool sf_gameplay_companion_pattern_visible(
    const SfNjpSparsePattern *pattern, SfScreenPoint anchor) {
  const int32_t left = anchor.x + pattern->image.x;
  const int32_t top = anchor.y + pattern->image.y;
  return left < (int32_t) SF_FRAME_WIDTH &&
    top < (int32_t) SF_FRAME_HEIGHT &&
    left + pattern->image.image.width > 0 &&
    top + pattern->image.image.height > 0;
}

static const SfCafSelectedAnimation *sf_gameplay_companion_animation(
    const SfCompanionAssets *assets, const SfCompanionState *companion) {
  if (!assets || !companion || !companion->valid ||
      companion->motion >= SF_COMPANION_ANIMATION_COUNT ||
      companion->direction >= SF_COMPANION_DIRECTION_COUNT) return NULL;
  return &assets->animations[companion->motion][companion->direction];
}

bool sf_gameplay_companion_visible(
    const SfCompanionAssets *assets, const SfCompanionState *companion,
    const SfWorldRenderView *view, uint16_t interpolation, bool shadow) {
  const SfCafSelectedAnimation *animation =
    sf_gameplay_companion_animation(assets, companion);
  const SfNjpSparseResource *resource;
  SfScreenPoint anchor;
  uint16_t frame;
  uint8_t part;
  if (!animation || !view || companion->current_life <= 0 ||
      animation->frame_count == 0u) return false;
  resource = shadow ? &assets->shadows : &assets->artwork;
  frame = (uint16_t) (companion->animation_frame % animation->frame_count);
  anchor = sf_world_to_screen(
    sf_companion_render_position(companion, interpolation));
  anchor.x -= view->camera_x;
  anchor.y -= view->camera_y;
  for (part = 0u; part < animation->part_count; ++part) {
    const SfCafCell *cell = &animation->parts[part].cells[frame];
    const SfNjpSparsePattern *pattern;
    if (cell->pattern < 0 || (shadow != ((cell->status & 8) != 0))) continue;
    pattern = sf_njp_sparse_pattern(resource, cell->pattern);
    if (pattern && sf_gameplay_companion_pattern_visible(pattern, anchor))
      return true;
  }
  return false;
}

void sf_gameplay_companion_draw(
    SfRenderer *renderer, const SfCompanionAssets *assets,
    const SfCompanionState *companion, const SfWorldRenderView *view,
    uint16_t interpolation, bool shadow, const SfRect *clip) {
  const SfCafSelectedAnimation *animation =
    sf_gameplay_companion_animation(assets, companion);
  const SfNjpSparseResource *resource;
  SfScreenPoint anchor;
  uint16_t frame;
  uint8_t priority;
  if (!renderer || !animation || !view || companion->current_life <= 0 ||
      animation->frame_count == 0u) return;
  resource = shadow ? &assets->shadows : &assets->artwork;
  frame = (uint16_t) (companion->animation_frame % animation->frame_count);
  anchor = sf_world_to_screen(
    sf_companion_render_position(companion, interpolation));
  anchor.x -= view->camera_x;
  anchor.y -= view->camera_y;
  for (priority = animation->priority_count; priority > 0u; --priority) {
    uint8_t part;
    for (part = 0u; part < animation->part_count; ++part) {
      const SfCafCell *cell = &animation->parts[part].cells[frame];
      const SfNjpSparsePattern *pattern;
      uint16_t opacity;
      SfBlendMode blend;
      if (cell->priority != (int16_t) (priority - 1u) || cell->pattern < 0 ||
          (shadow != ((cell->status & 8) != 0))) continue;
      pattern = sf_njp_sparse_pattern(resource, cell->pattern);
      if (!pattern) continue;
      opacity = shadow ? 500u :
        cell->transparency < 0 ? 0u :
        cell->transparency > 1000 ? 1000u : (uint16_t) cell->transparency;
      blend = shadow ? SF_BLEND_TRANSLUCENT :
        (cell->status & 0x10) != 0 ? SF_BLEND_ADDITIVE : SF_BLEND_MASKED;
      sf_renderer_draw_indexed_tinted(
        renderer, &pattern->image.image,
        anchor.x + pattern->image.x, anchor.y + pattern->image.y,
        shadow ? 1000u : (uint16_t) companion->profile.red_strength,
        shadow ? 1000u : (uint16_t) companion->profile.green_strength,
        shadow ? 1000u : (uint16_t) companion->profile.blue_strength,
        opacity, blend, clip);
    }
  }
}
