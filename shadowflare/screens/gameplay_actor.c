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

#include "screens/gameplay_actor.h"

#include "core/coordinates.h"
#include "core/memory_budget.h"

static bool sf_gameplay_actor_cell_visible(
    const SfNjpSparsePattern *pattern, SfScreenPoint anchor) {
  const int32_t left = anchor.x + pattern->image.x;
  const int32_t top = anchor.y + pattern->image.y;
  return left < (int32_t) SF_FRAME_WIDTH &&
    top < (int32_t) SF_FRAME_HEIGHT &&
    left + pattern->image.image.width > 0 &&
    top + pattern->image.image.height > 0;
}

bool sf_gameplay_actor_visible(
    const SfScenarioActorAssets *assets, const SfScenarioActor *actor,
    const SfWorldRenderView *view, bool shadow) {
  const SfScenarioActorVisual *visual;
  const SfCafSelectedAnimation *animation;
  const SfNjpSparseResource *resource;
  SfScreenPoint anchor;
  uint16_t frame;
  uint8_t part;
  if (!assets || !actor || !view || actor->direction >= 8u) return false;
  visual = sf_scenario_actor_visual(assets, actor->resource_id);
  if (!visual) return false;
  animation = &visual->animations[actor->direction];
  resource = shadow ? &visual->shadows : &visual->artwork;
  if (animation->frame_count == 0u) return false;
  frame = (uint16_t) (actor->animation_frame % animation->frame_count);
  anchor = sf_world_to_screen(actor->position);
  anchor.x -= view->camera_x;
  anchor.y -= view->camera_y;
  for (part = 0u; part < animation->part_count; ++part) {
    const uint8_t source_part = animation->parts[part].source_index;
    const SfCafCell *cell = &animation->parts[part].cells[frame];
    const SfNjpSparsePattern *pattern;
    if (source_part >= 8u ||
        (actor->enabled_parts & (uint8_t) (1u << source_part)) == 0u ||
        cell->pattern < 0 ||
        (shadow != ((cell->status & 8) != 0))) continue;
    pattern = sf_njp_sparse_pattern(resource, cell->pattern);
    if (pattern && sf_gameplay_actor_cell_visible(pattern, anchor))
      return true;
  }
  return false;
}

void sf_gameplay_actor_draw(
    SfRenderer *renderer, const SfScenarioActorAssets *assets,
    const SfScenarioActor *actor, const SfWorldRenderView *view,
    bool shadow, const SfRect *clip) {
  const SfScenarioActorVisual *visual;
  const SfCafSelectedAnimation *animation;
  const SfNjpSparseResource *resource;
  SfScreenPoint anchor;
  uint16_t frame;
  uint8_t priority;
  if (!renderer || !assets || !actor || !view || actor->direction >= 8u)
    return;
  visual = sf_scenario_actor_visual(assets, actor->resource_id);
  if (!visual) return;
  animation = &visual->animations[actor->direction];
  resource = shadow ? &visual->shadows : &visual->artwork;
  if (animation->frame_count == 0u) return;
  frame = (uint16_t) (actor->animation_frame % animation->frame_count);
  anchor = sf_world_to_screen(actor->position);
  anchor.x -= view->camera_x;
  anchor.y -= view->camera_y;
  for (priority = animation->priority_count; priority > 0u; --priority) {
    uint8_t part;
    for (part = 0u; part < animation->part_count; ++part) {
      const uint8_t source_part = animation->parts[part].source_index;
      const SfCafCell *cell = &animation->parts[part].cells[frame];
      const SfNjpSparsePattern *pattern;
      uint16_t opacity;
      uint16_t red_strength;
      uint16_t green_strength;
      uint16_t blue_strength;
      SfBlendMode blend;
      if (source_part >= 8u ||
          (actor->enabled_parts & (uint8_t) (1u << source_part)) == 0u ||
          cell->priority != (int16_t) (priority - 1u) || cell->pattern < 0 ||
          (shadow != ((cell->status & 8) != 0))) continue;
      pattern = sf_njp_sparse_pattern(resource, cell->pattern);
      if (!pattern) continue;
      if (shadow) {
        opacity = 500u;
        red_strength = 1000u;
        green_strength = 1000u;
        blue_strength = 1000u;
        blend = SF_BLEND_TRANSLUCENT;
      } else {
        opacity = cell->transparency < 0 ? 0u :
          cell->transparency > 1000 ? 1000u :
          (uint16_t) cell->transparency;
        red_strength = actor->red_strength[source_part] < 0 ? 0u :
          (uint16_t) actor->red_strength[source_part];
        green_strength = actor->green_strength[source_part] < 0 ? 0u :
          (uint16_t) actor->green_strength[source_part];
        blue_strength = actor->blue_strength[source_part] < 0 ? 0u :
          (uint16_t) actor->blue_strength[source_part];
        blend = (cell->status & 0x10) != 0
          ? SF_BLEND_ADDITIVE : SF_BLEND_MASKED;
      }
      sf_renderer_draw_indexed_tinted(
        renderer, &pattern->image.image,
        anchor.x + pattern->image.x, anchor.y + pattern->image.y,
        red_strength, green_strength, blue_strength,
        opacity, blend, clip);
    }
  }
}
