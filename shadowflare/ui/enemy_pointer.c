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

#include "ui/enemy_pointer.h"

#include "core/coordinates.h"

static uint8_t sf_enemy_pointer_pixel(
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

static bool sf_enemy_pointer_image_hit(
    const SfIndexedImage *image, int left, int top,
    int pointer_x, int pointer_y, int half_size, bool *exact) {
  int first_x = pointer_x - half_size;
  int first_y = pointer_y - half_size;
  int last_x = pointer_x + half_size + 1;
  int last_y = pointer_y + half_size + 1;
  int y;
  bool hit = false;
  if (first_x < left) first_x = left;
  if (first_y < top) first_y = top;
  if (last_x > left + image->width) last_x = left + image->width;
  if (last_y > top + image->height) last_y = top + image->height;
  for (y = first_y; y < last_y; ++y) {
    int x;
    for (x = first_x; x < last_x; ++x) {
      if (sf_enemy_pointer_pixel(
            image, (uint16_t) (x - left),
            (uint16_t) (y - top)) == 0u) continue;
      hit = true;
      if (exact && x == pointer_x && y == pointer_y) *exact = true;
    }
  }
  return hit;
}

bool sf_enemy_pointer_hit(
    const SfScenarioEnemyAssets *assets, const SfScenarioEnemy *enemy,
    const SfWorldRenderView *view, int pointer_x, int pointer_y,
    int half_size, bool *exact) {
  const SfScenarioEnemyVisual *visual;
  const SfCafSelectedAnimation *animation;
  SfScreenPoint anchor;
  uint16_t frame;
  uint8_t part;
  bool hit = false;
  if (exact) *exact = false;
  if (!assets || !enemy || !enemy->definition || !view ||
      enemy->current_life <= 0 ||
      !sf_scenario_enemy_state(enemy, SF_SCENARIO_VISIBLE) ||
      !sf_scenario_enemy_state(enemy, SF_SCENARIO_POINTER) ||
      enemy->direction >= SF_SCENARIO_ENEMY_DIRECTION_COUNT ||
      enemy->animation_chart >= SF_SCENARIO_ENEMY_ANIMATION_COUNT)
    return false;
  visual = sf_scenario_enemy_visual(
    assets, enemy->definition->resource_id);
  if (!visual) return false;
  animation = &visual->animations[enemy->animation_chart][enemy->direction];
  if (animation->frame_count == 0u) return false;
  frame = (uint16_t) (enemy->animation_frame % animation->frame_count);
  anchor = sf_world_to_screen(enemy->position);
  anchor.x -= view->camera_x;
  anchor.y -= view->camera_y;
  for (part = 0u; part < animation->part_count; ++part) {
    const uint8_t source_part = animation->parts[part].source_index;
    const SfCafCell *cell = &animation->parts[part].cells[frame];
    const SfNjpSparsePattern *pattern;
    if (source_part >= SF_MCT_PERSON_PART_LIMIT ||
        (enemy->enabled_parts & (uint8_t) (1u << source_part)) == 0u ||
        cell->pattern < 0 || (cell->status & 8) != 0) continue;
    pattern = sf_njp_sparse_pattern(&visual->artwork, cell->pattern);
    if (pattern && sf_enemy_pointer_image_hit(
          &pattern->image.image,
          anchor.x + pattern->image.x, anchor.y + pattern->image.y,
          pointer_x, pointer_y, half_size, exact)) hit = true;
  }
  return hit;
}
