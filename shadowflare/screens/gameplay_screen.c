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

#include "screens/gameplay_screen.h"

#include "core/coordinates.h"
#include "core/memory_budget.h"
#include "render/depth.h"

#include <string.h>

static const SfNjpDecodedPattern *sf_gameplay_object_pattern(
    const SfGameplayAssets *assets, const SfMapObject *object, bool shadow,
    const SfNjpDecodedResource **resource) {
  int set = object->pattern_set;
  if (shadow) ++set;
  if (set < 0 || set > UINT8_MAX || object->pattern < 0 ||
      object->pattern > UINT8_MAX) return NULL;
  *resource = sf_gameplay_pattern_set(assets, (uint8_t) set);
  return *resource
    ? sf_njp_decoded_pattern(*resource, (uint8_t) object->pattern) : NULL;
}

static uint16_t sf_gameplay_collect_objects(
    const SfGameplayAssets *assets, bool shadow,
    uint16_t *indices) {
  SfDepthEntry entries[SF_GAMEPLAY_VISIBLE_OBJECT_LIMIT];
  uint16_t count = 0u;
  uint16_t object_index;
  for (object_index = 0u; object_index < assets->objects.count;
       ++object_index) {
    const SfMapObject *object = &assets->objects.objects[object_index];
    const SfNjpDecodedResource *resource = NULL;
    if (shadow && (object->status & 8) == 0) continue;
    if (!sf_gameplay_object_pattern(
          assets, object, shadow, &resource)) continue;
    if (count >= SF_GAMEPLAY_VISIBLE_OBJECT_LIMIT) return UINT16_MAX;
    entries[count].position.x = object->world_x;
    entries[count].position.y = object->world_y;
    entries[count].judgement = object->judgement;
    entries[count].source_index = object_index;
    entries[count].status = object->status;
    ++count;
  }
  sf_depth_sort(entries, count);
  for (object_index = 0u; object_index < count; ++object_index)
    indices[object_index] = entries[object_index].source_index;
  return count;
}

bool sf_gameplay_screen_init(
    SfGameplayScreen *screen, const SfGameplayAssets *assets) {
  if (!screen || !assets) return false;
  memset(screen, 0, sizeof(*screen));
  screen->visible_count = sf_gameplay_collect_objects(
    assets, false, screen->visible_objects);
  screen->shadow_count = sf_gameplay_collect_objects(
    assets, true, screen->shadow_objects);
  if (screen->visible_count == UINT16_MAX ||
      screen->shadow_count == UINT16_MAX) {
    memset(screen, 0, sizeof(*screen));
    return false;
  }
  return true;
}

static void sf_gameplay_draw_pattern(
    SfRenderer *renderer, const SfNjpDecodedResource *resource,
    const SfNjpDecodedPattern *pattern, int x, int y,
    int palette_override, uint16_t opacity, SfBlendMode blend) {
  uint8_t palette;
  uint8_t reference;
  const uint16_t *colors;
  if (!resource || !pattern) return;
  palette = pattern->palette;
  if (palette >= resource->palette_count) return;
  colors = palette_override >= 0
    ? sf_njp_decoded_palette(resource, (uint16_t) palette_override)
    : resource->palettes[palette];
  if (!colors) colors = resource->palettes[palette];
  for (reference = 0u; reference < pattern->reference_count; ++reference) {
    const SfNjpDecodedReference *item =
      &resource->references[pattern->first_reference + reference];
    SfIndexedImage image;
    if (item->part >= resource->part_count) continue;
    image = resource->parts[item->part].image;
    image.palette = colors;
    sf_renderer_draw_indexed(
      renderer, &image, x + item->x, y + item->y,
      1000u, opacity, blend, NULL);
  }
}

static void sf_gameplay_draw_ground(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfWorldState *world) {
  const SfGroundMap *ground = &assets->ground;
  int32_t first_x = sf_floor_divide(world->camera_x, ground->chip_width);
  int32_t first_y = sf_floor_divide(world->camera_y, ground->chip_height);
  int32_t last_x = sf_floor_divide(
    world->camera_x + SF_FRAME_WIDTH - 1, ground->chip_width);
  int32_t last_y = sf_floor_divide(
    world->camera_y + SF_FRAME_HEIGHT - 1, ground->chip_height);
  int32_t y;
  if (first_x < 0) first_x = 0;
  if (first_y < 0) first_y = 0;
  if (last_x >= ground->width) last_x = ground->width - 1;
  if (last_y >= ground->height) last_y = ground->height - 1;
  for (y = first_y; y <= last_y; ++y) {
    int32_t x;
    for (x = first_x; x <= last_x; ++x) {
      const SfGroundCell *cell = sf_ground_cell(ground, x, y);
      const SfNjpDecodedResource *resource;
      const SfNjpDecodedPattern *pattern;
      if (!cell || cell->pattern_set == SF_GROUND_EMPTY_PATTERN ||
          cell->pattern == SF_GROUND_EMPTY_PATTERN) continue;
      resource = sf_gameplay_pattern_set(assets, cell->pattern_set);
      pattern = resource
        ? sf_njp_decoded_pattern(resource, cell->pattern) : NULL;
      sf_gameplay_draw_pattern(
        renderer, resource, pattern,
        x * ground->chip_width - world->camera_x,
        y * ground->chip_height - world->camera_y,
        -1, 1000u, SF_BLEND_OPAQUE);
    }
  }
}

static void sf_gameplay_draw_object(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfWorldState *world, uint16_t object_index, bool shadow) {
  const SfMapObject *object = &assets->objects.objects[object_index];
  const SfNjpDecodedResource *resource = NULL;
  const SfNjpDecodedPattern *pattern = sf_gameplay_object_pattern(
    assets, object, shadow, &resource);
  SfScreenPoint anchor;
  uint16_t opacity;
  SfBlendMode blend;
  if (!pattern) return;
  anchor = sf_world_to_screen(
    (SfWorldPoint) {object->world_x, object->world_y});
  if (!shadow) anchor.y -= object->height * 20 / 100;
  if (shadow) {
    opacity = 500u;
    blend = SF_BLEND_TRANSLUCENT;
  } else {
    opacity = object->opacity < 0 ? 0u :
      object->opacity > 1000 ? 1000u : (uint16_t) object->opacity;
    blend = (object->status & 0x10) != 0
      ? SF_BLEND_ADDITIVE : SF_BLEND_MASKED;
  }
  sf_gameplay_draw_pattern(
    renderer, resource, pattern,
    anchor.x - world->camera_x, anchor.y - world->camera_y,
    shadow ? -1 : object->palette, opacity, blend);
}

static void sf_gameplay_draw_object_pass(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfWorldState *world, const uint16_t *indices,
    uint16_t count, bool shadow, bool default_class) {
  uint16_t index;
  for (index = 0u; index < count; ++index) {
    const SfMapObject *object = &assets->objects.objects[indices[index]];
    if ((sf_depth_class(object->status) == 0) != default_class) continue;
    sf_gameplay_draw_object(
      renderer, assets, world, indices[index], shadow);
  }
}

void sf_gameplay_screen_draw(
    SfGameplayScreen *screen, SfRenderer *renderer,
    const SfGameplayAssets *assets, const SfGame *game) {
  if (!screen || !renderer || !assets || !game || screen->drawn ||
      !game->world.entered) return;
  sf_renderer_clear(renderer, 0u);
  sf_gameplay_draw_ground(renderer, assets, &game->world);
  sf_gameplay_draw_object_pass(
    renderer, assets, &game->world,
    screen->shadow_objects, screen->shadow_count, true, false);
  sf_gameplay_draw_object_pass(
    renderer, assets, &game->world,
    screen->visible_objects, screen->visible_count, false, false);
  sf_gameplay_draw_object_pass(
    renderer, assets, &game->world,
    screen->shadow_objects, screen->shadow_count, true, true);
  sf_gameplay_draw_object_pass(
    renderer, assets, &game->world,
    screen->visible_objects, screen->visible_count, false, true);
  screen->drawn = true;
}
