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
#include "screens/gameplay_object_visual.h"
#include "screens/gameplay_player.h"

#include <string.h>

#define SF_GAMEPLAY_PLAYER_ENTRY UINT16_MAX

static uint16_t sf_gameplay_collect_objects(
    const SfGameplayAssets *assets, const SfWorldState *world,
    const SfWorldRenderView *view, bool shadow, uint16_t *indices) {
  SfDepthEntry entries[SF_GAMEPLAY_DRAW_ENTRY_LIMIT];
  uint16_t count = 0u;
  uint16_t object_index;
  for (object_index = 0u; object_index < assets->objects.count;
       ++object_index) {
    const SfMapObject *object = &assets->objects.objects[object_index];
    SfGameplayObjectVisual visual;
    if (shadow && (object->status & 8) == 0) continue;
    if (!sf_gameplay_object_visual_find(
          assets, object, shadow, &visual) ||
        !sf_gameplay_object_visual_visible(
          &visual, object, view, shadow)) continue;
    if (count >= SF_GAMEPLAY_VISIBLE_OBJECT_LIMIT) return UINT16_MAX;
    entries[count].position.x = object->world_x;
    entries[count].position.y = object->world_y;
    entries[count].judgement = object->judgement;
    entries[count].source_index = object_index;
    entries[count].status = object->status;
    ++count;
  }
  if (world && world->entered && count < SF_GAMEPLAY_DRAW_ENTRY_LIMIT &&
      (!shadow || assets->player.shadows.pattern_count > 0u)) {
    entries[count].position = view->player_position;
    entries[count].judgement = world->player.judgement;
    entries[count].source_index = SF_GAMEPLAY_PLAYER_ENTRY;
    entries[count].status = 0;
    ++count;
  }
  sf_depth_sort(entries, count);
  for (object_index = 0u; object_index < count; ++object_index)
    indices[object_index] = entries[object_index].source_index;
  return count;
}

static void sf_gameplay_mark_translucent_objects(
    SfGameplayScreen *screen, const SfGameplayAssets *assets,
    const SfWorldRenderView *view) {
  SfScreenPoint player_screen;
  SfRect rectangle;
  uint16_t index;
  bool player_reached = false;
  memset(screen->translucent_objects, 0,
    sizeof(screen->translucent_objects));
  player_screen = sf_world_to_screen(view->player_position);
  player_screen.x -= view->camera_x;
  player_screen.y -= view->camera_y;
  rectangle.x = (int16_t) (player_screen.x - 25);
  rectangle.y = (int16_t) (player_screen.y - 60);
  rectangle.width = 51;
  rectangle.height = 61;
  for (index = 0u; index < screen->visible_count; ++index) {
    const uint16_t object_index = screen->visible_objects[index];
    const SfMapObject *object;
    SfGameplayObjectVisual visual;
    if (object_index == SF_GAMEPLAY_PLAYER_ENTRY) {
      player_reached = true;
      continue;
    }
    if (!player_reached) continue;
    object = &assets->objects.objects[object_index];
    if ((object->status & 0x2000) != 0 ||
        !sf_gameplay_object_visual_find(
          assets, object, false, &visual)) continue;
    if (sf_gameplay_object_visual_intersects(
          &visual, object, view, rectangle))
      screen->translucent_objects[index] = 1u;
  }
}

bool sf_gameplay_screen_init(
    SfGameplayScreen *screen, const SfGameplayAssets *assets,
    const SfWorldState *world) {
  SfWorldRenderView view;
  if (!screen || !assets || !world) return false;
  memset(screen, 0, sizeof(*screen));
  sf_world_render_view(world, 1000u, &view);
  screen->visible_count = sf_gameplay_collect_objects(
    assets, world, &view, false, screen->visible_objects);
  screen->shadow_count = sf_gameplay_collect_objects(
    assets, world, &view, true, screen->shadow_objects);
  if (screen->visible_count == UINT16_MAX ||
      screen->shadow_count == UINT16_MAX) {
    memset(screen, 0, sizeof(*screen));
    return false;
  }
  sf_gameplay_mark_translucent_objects(screen, assets, &view);
  screen->player_damage = sf_gameplay_player_bounds(
    &assets->player, world, &view);
  return true;
}

static void sf_gameplay_draw_pattern(
    SfRenderer *renderer, const SfNjpDecodedResource *resource,
    const SfNjpDecodedPattern *pattern, int x, int y,
    int palette_override, uint16_t opacity, SfBlendMode blend,
    const SfRect *clip) {
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
      1000u, opacity, blend, clip);
  }
}

static void sf_gameplay_draw_ground(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfWorldRenderView *view, const SfRect *clip) {
  const SfGroundMap *ground = &assets->ground;
  int32_t first_x = sf_floor_divide(view->camera_x, ground->chip_width);
  int32_t first_y = sf_floor_divide(view->camera_y, ground->chip_height);
  int32_t last_x = sf_floor_divide(
    view->camera_x + SF_FRAME_WIDTH - 1, ground->chip_width);
  int32_t last_y = sf_floor_divide(
    view->camera_y + SF_FRAME_HEIGHT - 1, ground->chip_height);
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
        x * ground->chip_width - view->camera_x,
        y * ground->chip_height - view->camera_y,
        -1, 1000u, SF_BLEND_OPAQUE, clip);
    }
  }
}

static void sf_gameplay_draw_object(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfWorldRenderView *view, uint16_t object_index, bool shadow,
    bool semi_transparent, const SfRect *clip) {
  const SfMapObject *object = &assets->objects.objects[object_index];
  SfGameplayObjectVisual visual;
  SfScreenPoint anchor;
  uint16_t opacity;
  SfBlendMode blend;
  if (!sf_gameplay_object_visual_find(
        assets, object, shadow, &visual)) return;
  anchor = sf_world_to_screen(
    (SfWorldPoint) {object->world_x, object->world_y});
  if (!shadow) anchor.y -= object->height * 20 / 100;
  if (shadow) {
    opacity = 500u;
    blend = SF_BLEND_TRANSLUCENT;
  } else {
    opacity = object->opacity < 0 ? 0u :
      object->opacity > 1000 ? 1000u : (uint16_t) object->opacity;
    if (semi_transparent && opacity > 500u) opacity = 500u;
    blend = (object->status & 0x10) != 0
      ? SF_BLEND_ADDITIVE : SF_BLEND_MASKED;
  }
  sf_gameplay_draw_pattern(
    renderer, visual.resource, visual.pattern,
    anchor.x - view->camera_x, anchor.y - view->camera_y,
    shadow ? -1 : object->palette, opacity, blend, clip);
}

static void sf_gameplay_draw_object_pass(
    SfRenderer *renderer, const SfGameplayAssets *assets,
    const SfWorldState *world, const SfWorldRenderView *view,
    const uint16_t *indices, const uint8_t *translucent,
    uint16_t count, bool shadow, bool default_class, const SfRect *clip) {
  uint16_t index;
  for (index = 0u; index < count; ++index) {
    if (indices[index] == SF_GAMEPLAY_PLAYER_ENTRY) {
      if (default_class)
        sf_gameplay_player_draw(
          renderer, &assets->player, world, view, shadow, clip);
    } else {
      const SfMapObject *object = &assets->objects.objects[indices[index]];
      if ((sf_depth_class(object->status) == 0) != default_class) continue;
      sf_gameplay_draw_object(
        renderer, assets, view, indices[index], shadow,
        translucent && translucent[index] != 0u, clip);
    }
  }
}

void sf_gameplay_screen_draw(
    SfGameplayScreen *screen, SfRenderer *renderer,
    const SfGameplayAssets *assets, const SfGame *game,
    uint16_t interpolation) {
  const SfRect *clip = NULL;
  const SfPlayerState *player;
  SfWorldRenderView view;
  bool scene_moved;
  if (!screen || !renderer || !assets || !game ||
      !game->world.entered) return;
  player = &game->world.player;
  sf_world_render_view(&game->world, interpolation, &view);
  scene_moved = !screen->drawn ||
    screen->rendered_player_x != view.player_position.x ||
    screen->rendered_player_y != view.player_position.y ||
    screen->rendered_camera_x != view.camera_x ||
    screen->rendered_camera_y != view.camera_y ||
    screen->rendered_motion != (uint8_t) player->motion ||
    screen->rendered_direction != player->direction;
  if (screen->drawn && !scene_moved) {
    if (screen->rendered_animation_frame ==
        player->animation_frame) return;
    clip = &screen->player_damage;
    sf_renderer_fill_rect(renderer, *clip, 0u);
  } else {
    screen->visible_count = sf_gameplay_collect_objects(
      assets, &game->world, &view, false, screen->visible_objects);
    screen->shadow_count = sf_gameplay_collect_objects(
      assets, &game->world, &view, true, screen->shadow_objects);
    if (screen->visible_count == UINT16_MAX ||
        screen->shadow_count == UINT16_MAX) return;
    sf_gameplay_mark_translucent_objects(screen, assets, &view);
    screen->player_damage = sf_gameplay_player_bounds(
      &assets->player, &game->world, &view);
    sf_renderer_clear(renderer, 0u);
  }
  sf_gameplay_draw_ground(renderer, assets, &view, clip);
  sf_gameplay_draw_object_pass(
    renderer, assets, &game->world, &view,
    screen->shadow_objects, NULL,
    screen->shadow_count, true, false, clip);
  sf_gameplay_draw_object_pass(
    renderer, assets, &game->world, &view,
    screen->visible_objects, screen->translucent_objects,
    screen->visible_count, false, false, clip);
  sf_gameplay_draw_object_pass(
    renderer, assets, &game->world, &view,
    screen->shadow_objects, NULL,
    screen->shadow_count, true, true, clip);
  sf_gameplay_draw_object_pass(
    renderer, assets, &game->world, &view,
    screen->visible_objects, screen->translucent_objects,
    screen->visible_count, false, true, clip);
  screen->rendered_animation_frame = player->animation_frame;
  screen->rendered_player_x = view.player_position.x;
  screen->rendered_player_y = view.player_position.y;
  screen->rendered_camera_x = view.camera_x;
  screen->rendered_camera_y = view.camera_y;
  screen->rendered_motion = (uint8_t) player->motion;
  screen->rendered_direction = player->direction;
  screen->drawn = true;
}
