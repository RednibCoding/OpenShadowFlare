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

#include "ui/world_pointer.h"

#include "core/coordinates.h"

#include <limits.h>

typedef struct SfActorPointerHit {
  bool intersects;
  bool exact;
} SfActorPointerHit;

static uint8_t sf_world_pointer_pixel(
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

static SfActorPointerHit sf_world_pointer_image_hit(
    const SfIndexedImage *image, int left, int top,
    int pointer_x, int pointer_y, int half_size) {
  SfActorPointerHit hit = {false, false};
  int first_x = pointer_x - half_size;
  int first_y = pointer_y - half_size;
  int last_x = pointer_x + half_size + 1;
  int last_y = pointer_y + half_size + 1;
  int y;
  if (first_x < left) first_x = left;
  if (first_y < top) first_y = top;
  if (last_x > left + image->width) last_x = left + image->width;
  if (last_y > top + image->height) last_y = top + image->height;
  for (y = first_y; y < last_y; ++y) {
    int x;
    for (x = first_x; x < last_x; ++x) {
      if (sf_world_pointer_pixel(
            image, (uint16_t) (x - left),
            (uint16_t) (y - top)) == 0u) continue;
      hit.intersects = true;
      if (x == pointer_x && y == pointer_y) hit.exact = true;
    }
  }
  return hit;
}

static SfActorPointerHit sf_world_pointer_actor_hit(
    const SfScenarioActorAssets *assets, const SfScenarioActor *actor,
    const SfWorldRenderView *view, int pointer_x, int pointer_y,
    int half_size) {
  SfActorPointerHit result = {false, false};
  const SfScenarioActorVisual *visual;
  const SfCafSelectedAnimation *animation;
  SfScreenPoint anchor;
  uint16_t frame;
  uint8_t part;
  if (!assets || !actor || actor->direction >= 8u ||
      actor->animation_chart >= SF_SCENARIO_ACTOR_ANIMATION_COUNT)
    return result;
  visual = sf_scenario_actor_visual(assets, actor->resource_id);
  if (!visual) return result;
  animation = &visual->animations[actor->animation_chart][actor->direction];
  if (animation->frame_count == 0u) return result;
  frame = (uint16_t) (actor->animation_frame % animation->frame_count);
  anchor = sf_world_to_screen(actor->position);
  anchor.x -= view->camera_x;
  anchor.y -= view->camera_y;
  for (part = 0u; part < animation->part_count; ++part) {
    const uint8_t source_part = animation->parts[part].source_index;
    const SfCafCell *cell = &animation->parts[part].cells[frame];
    const SfNjpSparsePattern *pattern;
    SfActorPointerHit part_hit;
    if (source_part >= 8u ||
        (actor->enabled_parts & (uint8_t) (1u << source_part)) == 0u ||
        cell->pattern < 0 || (cell->status & 8) != 0) continue;
    pattern = sf_njp_sparse_pattern(&visual->artwork, cell->pattern);
    if (!pattern) continue;
    part_hit = sf_world_pointer_image_hit(
      &pattern->image.image,
      anchor.x + pattern->image.x, anchor.y + pattern->image.y,
      pointer_x, pointer_y, half_size);
    if (part_hit.intersects) result.intersects = true;
    if (part_hit.exact) result.exact = true;
  }
  return result;
}

static int sf_world_pointer_half_size(const SfWorldPointerControl *pointer) {
  static const uint8_t half_sizes[5] = {0u, 12u, 16u, 24u, 48u};
  return pointer->range_enabled && pointer->range < 5u
    ? half_sizes[pointer->range] : 0;
}

void sf_world_pointer_resolve(
    const SfGameplayAssets *assets, const SfWorldState *world,
    SfGameInput *input) {
  SfWorldRenderView view;
  SfWorldPoint pointer_world;
  int64_t best_distance = INT64_MAX;
  bool best_exact = false;
  uint8_t index;
  if (!input) return;
  input->world_pointer_resolved = true;
  input->pointed_actor_id = -1;
  if (!assets || !world || !world->entered || !input->pointer_active ||
      input->pointer_y >= 408) return;
  sf_world_render_view(world, 1000u, &view);
  pointer_world = sf_screen_to_world((SfScreenPoint) {
    input->pointer_x + view.camera_x,
    input->pointer_y + view.camera_y});
  for (index = 0u; index < world->actors.count; ++index) {
    const SfScenarioActor *actor = &world->actors.actors[index];
    const SfActorPointerHit hit = sf_world_pointer_actor_hit(
      &assets->actors, actor, &view, input->pointer_x, input->pointer_y,
      sf_world_pointer_half_size(&world->pointer));
    const int64_t dx = (int64_t) actor->position.x - pointer_world.x;
    const int64_t dy = (int64_t) actor->position.y - pointer_world.y;
    const int64_t distance = dx * dx + dy * dy;
    if (!sf_scenario_actor_state(actor, SF_SCENARIO_VISIBLE) ||
        !sf_scenario_actor_state(actor, SF_SCENARIO_POINTER) ||
        !hit.intersects) continue;
    if (input->pointed_actor_id < 0 ||
        (hit.exact != best_exact ? hit.exact : distance <= best_distance)) {
      input->pointed_actor_id = actor->id;
      best_exact = hit.exact;
      best_distance = distance;
    }
  }
}
