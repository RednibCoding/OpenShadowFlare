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

#include "screens/gameplay_enemy.h"

#include "core/coordinates.h"
#include "core/memory_budget.h"

static bool sf_gameplay_enemy_cell_visible(
    const SfNjpSparsePattern *pattern, SfScreenPoint anchor) {
  const int32_t left = anchor.x + pattern->image.x;
  const int32_t top = anchor.y + pattern->image.y;
  return left < (int32_t) SF_FRAME_WIDTH &&
    top < (int32_t) SF_FRAME_HEIGHT &&
    left + pattern->image.image.width > 0 &&
    top + pattern->image.image.height > 0;
}

bool sf_gameplay_enemy_visible(
    const SfScenarioEnemyAssets *assets, const SfScenarioEnemy *enemy,
    const SfWorldRenderView *view, uint16_t interpolation, bool shadow) {
  const SfScenarioEnemyVisual *visual;
  const SfCafSelectedAnimation *animation;
  const SfNjpSparseResource *resource;
  SfScreenPoint anchor;
  uint16_t frame;
  uint8_t part;
  if (!assets || !enemy || !enemy->definition || !view ||
      enemy->direction >= SF_SCENARIO_ENEMY_DIRECTION_COUNT) return false;
  visual = sf_scenario_enemy_visual(
    assets, enemy->definition->resource_id);
  if (!visual) return false;
  if (enemy->animation_chart >= SF_SCENARIO_ENEMY_ANIMATION_COUNT)
    return false;
  animation = &visual->animations[enemy->animation_chart][enemy->direction];
  resource = shadow ? &visual->shadows : &visual->artwork;
  if (animation->frame_count == 0u) return false;
  frame = (uint16_t) (enemy->animation_frame % animation->frame_count);
  anchor = sf_world_to_screen(
    sf_scenario_enemy_render_position(enemy, interpolation));
  anchor.x -= view->camera_x;
  anchor.y -= view->camera_y;
  for (part = 0u; part < animation->part_count; ++part) {
    const uint8_t source_part = animation->parts[part].source_index;
    const SfCafCell *cell = &animation->parts[part].cells[frame];
    const SfNjpSparsePattern *pattern;
    if (source_part >= SF_MCT_PERSON_PART_LIMIT ||
        (enemy->enabled_parts & (uint8_t) (1u << source_part)) == 0u ||
        cell->pattern < 0 ||
        (shadow != ((cell->status & 8) != 0))) continue;
    pattern = sf_njp_sparse_pattern(resource, cell->pattern);
    if (pattern && sf_gameplay_enemy_cell_visible(pattern, anchor))
      return true;
  }
  return false;
}

void sf_gameplay_enemy_draw(
    SfRenderer *renderer, const SfScenarioEnemyAssets *assets,
    const SfScenarioEnemy *enemy, const SfWorldRenderView *view,
    uint16_t interpolation, bool shadow, const SfRect *clip) {
  const SfScenarioEnemyVisual *visual;
  const SfCafSelectedAnimation *animation;
  const SfNjpSparseResource *resource;
  SfScreenPoint anchor;
  uint16_t frame;
  uint8_t priority;
  if (!renderer || !assets || !enemy || !enemy->definition || !view ||
      enemy->direction >= SF_SCENARIO_ENEMY_DIRECTION_COUNT) return;
  visual = sf_scenario_enemy_visual(
    assets, enemy->definition->resource_id);
  if (!visual) return;
  if (enemy->animation_chart >= SF_SCENARIO_ENEMY_ANIMATION_COUNT) return;
  animation = &visual->animations[enemy->animation_chart][enemy->direction];
  resource = shadow ? &visual->shadows : &visual->artwork;
  if (animation->frame_count == 0u) return;
  frame = (uint16_t) (enemy->animation_frame % animation->frame_count);
  anchor = sf_world_to_screen(
    sf_scenario_enemy_render_position(enemy, interpolation));
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
      if (source_part >= SF_MCT_PERSON_PART_LIMIT ||
          (enemy->enabled_parts & (uint8_t) (1u << source_part)) == 0u ||
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
        red_strength = enemy->definition->red_strength[source_part] < 0
          ? 0u : (uint16_t) enemy->definition->red_strength[source_part];
        green_strength = enemy->definition->green_strength[source_part] < 0
          ? 0u : (uint16_t) enemy->definition->green_strength[source_part];
        blue_strength = enemy->definition->blue_strength[source_part] < 0
          ? 0u : (uint16_t) enemy->definition->blue_strength[source_part];
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
