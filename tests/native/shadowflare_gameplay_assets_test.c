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

#include "assets/gameplay_assets.h"
#include "core/coordinates.h"
#include "core/memory_budget.h"
#include "data/pattern_list.h"
#include "game/player.h"
#include "game/world.h"
#include "screens/gameplay_screen.h"
#include "screens/gameplay_object_visual.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef union SfGameplayTestMemory {
  long double alignment;
  void *pointer;
  uint8_t bytes[SF_MAIN_ARENA_BYTES];
} SfGameplayTestMemory;

static SfGameplayTestMemory sf_gameplay_test_memory;

static int is_shadow_name(const char *name) {
  const size_t length = name ? strlen(name) : 0u;
  return length >= 4u && name[length - 4u] == '.' &&
    tolower((unsigned char) name[length - 3u]) == 's' &&
    tolower((unsigned char) name[length - 2u]) == 'd' &&
    tolower((unsigned char) name[length - 1u]) == 'w';
}

static int check_pattern(
    const SfGameplayAssets *assets, int32_t set, int32_t pattern,
    const char *kind) {
  const SfNjpDecodedResource *resource;
  const SfNjpDecodedPattern *decoded;
  if (set < 0 || set > UINT8_MAX || pattern < 0 || pattern > UINT8_MAX)
    return 0;
  resource = sf_gameplay_pattern_set(assets, (uint8_t) set);
  decoded = resource
    ? sf_njp_decoded_pattern(resource, (uint8_t) pattern) : NULL;
  if (decoded && decoded->bounds.valid) return 0;
  fprintf(stderr, "Remote Town is missing %s pattern %d:%d\n",
    kind, (int) set, (int) pattern);
  return 1;
}

static int test_object_intersection(const SfGameplayAssets *assets) {
  uint16_t index;
  for (index = 0u; index < assets->objects.count; ++index) {
    const SfMapObject *object = &assets->objects.objects[index];
    SfGameplayObjectVisual visual;
    SfWorldRenderView view;
    SfScreenPoint anchor;
    SfRect rectangle;
    if (!sf_gameplay_object_visual_find(
          assets, object, false, &visual) ||
        !visual.pattern->bounds.valid || visual.pattern->bounds.width > 1000 ||
        visual.pattern->bounds.height > 1000) continue;
    anchor = sf_world_to_screen(
      (SfWorldPoint) {object->world_x, object->world_y});
    view.player_position = (SfWorldPoint) {object->world_x, object->world_y};
    view.camera_x = anchor.x - 320;
    view.camera_y = anchor.y - 240;
    rectangle.x = (int16_t) (320 + visual.pattern->bounds.x);
    rectangle.y = (int16_t) (
      240 + visual.pattern->bounds.y - object->height * 20 / 100);
    rectangle.width = (int16_t) visual.pattern->bounds.width;
    rectangle.height = (int16_t) visual.pattern->bounds.height;
    if (sf_gameplay_object_visual_intersects(
          &visual, object, &view, rectangle)) return 0;
  }
  fprintf(stderr, "Remote Town object pixels did not intersect their bounds\n");
  return 1;
}

int main(void) {
#if defined(OPENSHADOWFLARE_SOURCE_DIR)
  SfGameplayAssets assets;
  SfGameplayScreen screen;
  SfPatternList patterns;
  SfPlayerState player;
  SfWorldState world;
  SfArena arena;
  char root[1024];
  char probe_path[1024];
  FILE *probe;
  int32_t y;
  uint16_t index;
  (void) snprintf(
    root, sizeof(root), "%s/tmp/ShadowFlare", OPENSHADOWFLARE_SOURCE_DIR);
  (void) snprintf(
    probe_path, sizeof(probe_path),
    "%s/tmp/ShadowFlare/Map/Ground/f00_01.Gnd",
    OPENSHADOWFLARE_SOURCE_DIR);
  probe = fopen(probe_path, "rb");
  if (!probe) return 0;
  fclose(probe);
  (void) snprintf(
    probe_path, sizeof(probe_path),
    "%s/tmp/ShadowFlare/Map/Pattern/f00_01.Lst",
    OPENSHADOWFLARE_SOURCE_DIR);
  if (!sf_pattern_list_load(probe_path, &patterns)) return 1;
  sf_arena_init(
    &arena, sf_gameplay_test_memory.bytes,
    sizeof(sf_gameplay_test_memory.bytes));
  sf_player_init(&player, 1u);
  if (!sf_gameplay_assets_load(
        &assets, root, 0, 0, player.gender,
        player.appearance_parts, player.appearance_part_count,
        player.visible_items, player.visible_item_count, &arena)) {
    fprintf(stderr, "Remote Town gameplay assets did not fit the game arena\n");
    return 1;
  }
  for (index = 0u; index < assets.pattern_set_count; ++index) {
    const SfGameplayPatternSet *set = &assets.pattern_sets[index];
    if (set->source_index >= patterns.count ||
        set->resource.is_shadow !=
          (is_shadow_name(patterns.names[set->source_index]) != 0)) {
      fprintf(stderr, "Remote Town pattern-set type does not match its NJP\n");
      return 1;
    }
  }
  for (y = 0; y < assets.ground.height; ++y) {
    int32_t x;
    for (x = 0; x < assets.ground.width; ++x) {
      const SfGroundCell *cell = sf_ground_cell(&assets.ground, x, y);
      if (!cell || cell->pattern_set == SF_GROUND_EMPTY_PATTERN ||
          cell->pattern == SF_GROUND_EMPTY_PATTERN) continue;
      if (check_pattern(
            &assets, cell->pattern_set, cell->pattern, "ground")) return 1;
    }
  }
  for (index = 0u; index < assets.objects.count; ++index) {
    const SfMapObject *object = &assets.objects.objects[index];
    if (check_pattern(
          &assets, object->pattern_set, object->pattern, "object")) return 1;
    if ((object->status & 8) != 0 && object->pattern_set >= 0 &&
        object->pattern_set + 1 < patterns.count && object->pattern >= 0 &&
        object->pattern <= UINT8_MAX &&
        is_shadow_name(patterns.names[object->pattern_set + 1])) {
      const SfNjpDecodedResource *shadow = sf_gameplay_pattern_set(
        &assets, (uint8_t) (object->pattern_set + 1));
      if (shadow && !sf_njp_decoded_pattern(
            shadow, (uint8_t) object->pattern)) {
        fprintf(stderr, "Remote Town is missing shadow pattern %d:%d\n",
          object->pattern_set + 1, object->pattern);
        return 1;
      }
    }
  }
  if (test_object_intersection(&assets)) return 1;
  sf_world_state_init(&world, 0, 0, player.gender);
  sf_world_state_bind_collision(&world, &assets.ground, &assets.objects);
  sf_world_state_enter(
    &world, assets.entry.world_x, assets.entry.world_y,
    (uint8_t) assets.entry.direction);
  if (!sf_gameplay_screen_init(&screen, &assets, &world) ||
      screen.visible_count != 22u || screen.shadow_count != 10u) {
    fprintf(stderr,
      "Remote Town viewport culling changed before retail depth sorting\n");
    return 1;
  }
#endif
  return 0;
}
