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

#include "screens/gameplay_scene.h"

#include "core/coordinates.h"
#include "core/memory_budget.h"
#include "render/depth.h"
#include "screens/gameplay_actor.h"
#include "screens/gameplay_companion.h"
#include "screens/gameplay_ground_item.h"
#include "screens/gameplay_object_visual.h"
#include "screens/gameplay_player.h"

#include <string.h>

#define SF_GAMEPLAY_PLAYER_ENTRY UINT16_MAX
#define SF_GAMEPLAY_COMPANION_ENTRY UINT16_C(0x3fff)
#define SF_GAMEPLAY_GROUND_ITEM_ENTRY_BASE UINT16_C(0x4000)
#define SF_GAMEPLAY_ACTOR_ENTRY_BASE UINT16_C(0x8000)

static bool sf_gameplay_ground_item_entry(uint16_t entry) {
  return entry >= SF_GAMEPLAY_GROUND_ITEM_ENTRY_BASE &&
    entry < SF_GAMEPLAY_GROUND_ITEM_ENTRY_BASE + SF_GROUND_ITEM_LIMIT;
}

static bool sf_gameplay_actor_entry(uint16_t entry) {
  return entry >= SF_GAMEPLAY_ACTOR_ENTRY_BASE &&
    entry < SF_GAMEPLAY_ACTOR_ENTRY_BASE + SF_MCT_PERSON_LIMIT;
}

static uint16_t sf_gameplay_collect_objects(
    const SfGameplayAssets *assets, const SfWorldState *world,
    const SfWorldRenderView *view, uint16_t interpolation,
    bool shadow, uint16_t *indices) {
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
  if (world && world->companion.valid && world->companion.current_life > 0 &&
      sf_gameplay_companion_visible(
        &assets->companion, &world->companion, view, interpolation, shadow)) {
    if (count >= SF_GAMEPLAY_DRAW_ENTRY_LIMIT) return UINT16_MAX;
    entries[count].position = sf_companion_render_position(
      &world->companion, interpolation);
    entries[count].judgement = world->companion.judgement;
    entries[count].source_index = SF_GAMEPLAY_COMPANION_ENTRY;
    entries[count].status = 0;
    ++count;
  }
  if (world) {
    uint8_t item_index;
    for (item_index = 0u; item_index < world->ground_items.count;
         ++item_index) {
      const SfGroundItem *item = &world->ground_items.items[item_index];
      if (!sf_gameplay_ground_item_visible(
            &assets->ground_items, item, view, shadow)) continue;
      if (count >= SF_GAMEPLAY_DRAW_ENTRY_LIMIT) return UINT16_MAX;
      entries[count].position = item->position;
      entries[count].judgement = item->judgement;
      entries[count].source_index = (uint16_t) (
        SF_GAMEPLAY_GROUND_ITEM_ENTRY_BASE + item_index);
      entries[count].status = 0;
      ++count;
    }
    uint8_t actor_index;
    for (actor_index = 0u; actor_index < world->actors.count;
         ++actor_index) {
      const SfScenarioActor *actor = &world->actors.actors[actor_index];
      const SfScenarioActorVisual *visual;
      if (!sf_scenario_actor_state(actor, SF_SCENARIO_VISIBLE)) continue;
      visual = sf_scenario_actor_visual(&assets->actors, actor->resource_id);
      if (!visual || (shadow && visual->shadows.pattern_count == 0u) ||
          !sf_gameplay_actor_visible(
            &assets->actors, actor, view, interpolation, shadow)) continue;
      if (count >= SF_GAMEPLAY_DRAW_ENTRY_LIMIT) return UINT16_MAX;
      entries[count].position = sf_scenario_actor_render_position(
        actor, interpolation);
      entries[count].judgement = actor->judgement;
      entries[count].source_index = (uint16_t) (
        SF_GAMEPLAY_ACTOR_ENTRY_BASE + actor_index);
      entries[count].status = 0;
      ++count;
    }
  }
  sf_depth_sort(entries, count);
  for (object_index = 0u; object_index < count; ++object_index)
    indices[object_index] = entries[object_index].source_index;
  return count;
}

static void sf_gameplay_mark_translucent_objects(
    SfGameplayScene *scene, const SfGameplayAssets *assets,
    const SfWorldRenderView *view) {
  SfScreenPoint player_screen;
  SfRect rectangle;
  uint16_t index;
  bool player_reached = false;
  memset(scene->translucent_objects, 0, sizeof(scene->translucent_objects));
  player_screen = sf_world_to_screen(view->player_position);
  player_screen.x -= view->camera_x;
  player_screen.y -= view->camera_y;
  rectangle.x = (int16_t) (player_screen.x - 25);
  rectangle.y = (int16_t) (player_screen.y - 60);
  rectangle.width = 51;
  rectangle.height = 61;
  for (index = 0u; index < scene->visible_count; ++index) {
    const uint16_t object_index = scene->visible_objects[index];
    const SfMapObject *object;
    SfGameplayObjectVisual visual;
    if (object_index == SF_GAMEPLAY_PLAYER_ENTRY) {
      player_reached = true;
      continue;
    }
    if (object_index == SF_GAMEPLAY_COMPANION_ENTRY ||
        sf_gameplay_actor_entry(object_index) ||
        sf_gameplay_ground_item_entry(object_index) ||
        !player_reached) continue;
    object = &assets->objects.objects[object_index];
    if ((object->status & 0x2000) != 0 ||
        !sf_gameplay_object_visual_find(
          assets, object, false, &visual)) continue;
    if (sf_gameplay_object_visual_intersects(
          &visual, object, view, rectangle))
      scene->translucent_objects[index] = 1u;
  }
}

bool sf_gameplay_scene_update(
    SfGameplayScene *scene, const SfGameplayAssets *assets,
    const SfWorldState *world, const SfWorldRenderView *view,
    uint16_t interpolation) {
  if (!scene || !assets || !world || !view) return false;
  scene->visible_count = sf_gameplay_collect_objects(
    assets, world, view, interpolation, false, scene->visible_objects);
  scene->shadow_count = sf_gameplay_collect_objects(
    assets, world, view, interpolation, true, scene->shadow_objects);
  if (scene->visible_count == UINT16_MAX ||
      scene->shadow_count == UINT16_MAX) return false;
  sf_gameplay_mark_translucent_objects(scene, assets, view);
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
    uint16_t count, uint16_t interpolation,
    bool shadow, bool default_class, const SfRect *clip) {
  uint16_t index;
  for (index = 0u; index < count; ++index) {
    if (indices[index] == SF_GAMEPLAY_PLAYER_ENTRY) {
      if (default_class)
        sf_gameplay_player_draw(
          renderer, &assets->player, world, view, shadow, clip);
    } else if (indices[index] == SF_GAMEPLAY_COMPANION_ENTRY) {
      if (default_class)
        sf_gameplay_companion_draw(
          renderer, &assets->companion, &world->companion,
          view, interpolation, shadow, clip);
    } else if (sf_gameplay_actor_entry(indices[index])) {
      const uint16_t actor_index = (uint16_t) (
        indices[index] - SF_GAMEPLAY_ACTOR_ENTRY_BASE);
      const SfScenarioActor *actor = sf_scenario_actor_at(
        &world->actors, (uint8_t) actor_index);
      if (default_class && actor)
        sf_gameplay_actor_draw(
          renderer, &assets->actors, actor, view,
          interpolation, shadow,
          !shadow && world->pointer.hovered_actor_id == actor->id,
          clip);
    } else if (sf_gameplay_ground_item_entry(indices[index])) {
      const uint16_t item_index = (uint16_t) (
        indices[index] - SF_GAMEPLAY_GROUND_ITEM_ENTRY_BASE);
      if (default_class && item_index < world->ground_items.count)
        sf_gameplay_ground_item_draw(
          renderer, &assets->ground_items,
          &world->ground_items.items[item_index], view, shadow,
          !shadow && world->pointer.hovered_ground_item_id ==
            world->ground_items.items[item_index].id,
          clip);
    } else {
      const SfMapObject *object = &assets->objects.objects[indices[index]];
      if ((sf_depth_class(object->status) == 0) != default_class) continue;
      sf_gameplay_draw_object(
        renderer, assets, view, indices[index], shadow,
        translucent && translucent[index] != 0u, clip);
    }
  }
}

void sf_gameplay_scene_draw(
    const SfGameplayScene *scene, SfRenderer *renderer,
    const SfGameplayAssets *assets, const SfWorldState *world,
    const SfWorldRenderView *view, uint16_t interpolation,
    const SfRect *clip) {
  if (!scene || !renderer || !assets || !world || !view) return;
  sf_gameplay_draw_ground(renderer, assets, view, clip);
  sf_gameplay_draw_object_pass(
    renderer, assets, world, view, scene->shadow_objects, NULL,
    scene->shadow_count, interpolation, true, false, clip);
  sf_gameplay_draw_object_pass(
    renderer, assets, world, view, scene->visible_objects,
    scene->translucent_objects, scene->visible_count,
    interpolation, false, false, clip);
  sf_gameplay_draw_object_pass(
    renderer, assets, world, view, scene->shadow_objects, NULL,
    scene->shadow_count, interpolation, true, true, clip);
  sf_gameplay_draw_object_pass(
    renderer, assets, world, view, scene->visible_objects,
    scene->translucent_objects, scene->visible_count,
    interpolation, false, true, clip);
}
