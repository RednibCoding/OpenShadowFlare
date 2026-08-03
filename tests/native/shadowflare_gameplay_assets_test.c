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
#include "ui/conversation_layout.h"
#include "ui/world_pointer.h"

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

static int test_scenario_actors(
    const SfGameplayAssets *assets, SfWorldState *world) {
  static const char *expected_names[7] = {
    "Ostare", "Malse", "Syria", "Kerberos", "Gravity", "Dune", "Harley"
  };
  static const int32_t expected_resources[7] = {
    13, 8, 9, 1000000, 1000000, 1000000, 1000001
  };
  uint8_t index;
  SfCollisionQuery collision;
  if (assets->scenario.people_count != 7u ||
      assets->scenario.object_count != 7u ||
      assets->scenario.objects[3].id != 202 ||
      assets->scenario.objects[3].world_x != 94620 ||
      assets->scenario.objects[3].world_y != -4043 ||
      assets->actors.visual_count != 5u || !assets->script ||
      assets->script->temporary_flag_count != 66u ||
      assets->script->status_count != 23u ||
      assets->script->sentence_count != 220u ||
      assets->script->command_count != 608u ||
      assets->script->operand_count != 1895u ||
      assets->script->message_count != 61u ||
      strcmp(sf_scs_message_text(
          assets->script, sf_scs_message(assets->script, 1000000)),
        "Thank you for coming. I am Ostare, a commander of this area.\n"
        "As you see, this town has fallen into a critical state.\n\201@\n"
        "I can't sleep peacefully because goblins and demons are\n"
        "lingering around the protective wall.\n") != 0) {
    fprintf(stderr, "Remote Town actor or script counts differ from retail\n");
    return 1;
  }
  for (index = 0u; index < 7u; ++index) {
    const SfMctPerson *person = &assets->scenario.people[index];
    const SfScenarioActorVisual *visual = sf_scenario_actor_visual(
      &assets->actors, expected_resources[index]);
    uint8_t direction;
    if (strcmp(person->name, expected_names[index]) != 0 ||
        person->resource_id != expected_resources[index] || !visual ||
        visual->artwork.pattern_count == 0u ||
        visual->shadows.pattern_count == 0u) {
      fprintf(stderr, "Remote Town PEOPLE resource %u differs from retail\n",
        (unsigned) index);
      return 1;
    }
    for (direction = 0u; direction < 8u; ++direction) {
      if (visual->animations[0][direction].frame_count == 0u) {
        fprintf(stderr, "Remote Town actor %u has an empty idle direction\n",
          (unsigned) index);
        return 1;
      }
    }
  }
  if (assets->scenario.people[0].id != 0 ||
      assets->scenario.people[0].world_x != 91467 ||
      assets->scenario.people[0].world_y != 1532 ||
      assets->scenario.people[0].direction != 7 ||
      assets->scenario.people[0].part_visibility[4] != 0u ||
      assets->scenario.people[0].part_visibility[5] != 0u ||
      assets->scenario.people[0].part_visibility[6] == 0u ||
      !sf_world_state_bind_scenario(
        world, &assets->scenario, assets->script) ||
      world->actors.count != 7u ||
      world->actors.actors[4].red_strength[1] != 400 ||
      world->actors.actors[4].green_strength[1] != 400 ||
      world->actors.actors[4].blue_strength[1] != 400 ||
      world->actors.actors[5].red_strength[1] != 900 ||
      world->actors.actors[5].green_strength[1] != 800 ||
      world->actors.actors[5].blue_strength[1] != 700) {
    fprintf(stderr, "Ostare or the actor runtime differs from retail\n");
    return 1;
  }
  for (index = 0u; index < 7u; ++index) {
    const SfScenarioActor *actor = &world->actors.actors[index];
    const bool expected_visible = index != 3u;
    if (sf_scenario_actor_state(actor, SF_SCENARIO_VISIBLE) !=
          expected_visible ||
        sf_scenario_actor_state(actor, SF_SCENARIO_POINTER) !=
          expected_visible ||
        sf_scenario_actor_state(actor, SF_SCENARIO_JUDGEMENT) !=
          expected_visible) {
      fprintf(stderr,
        "Remote Town companion visibility was not projected from SCS\n");
      return 1;
    }
  }
  if (sf_scenario_actor_visual(&assets->actors, 13)->animations[1][0].frame_count
        == 0u ||
      sf_scenario_actor_visual(&assets->actors, 8)->animations[1][0].frame_count
        != 0u) {
    fprintf(stderr, "Remote Town actor walk assets are not loaded sparsely\n");
    return 1;
  }
  collision.world = &world->collision;
  collision.blockers = NULL;
  collision.ignored_blocker_id = 0;
  collision.blocker_count = 0u;
  sf_scenario_actor_update(&world->actors.actors[0], &collision);
  if (world->actors.actors[0].animation_chart != 0u ||
      world->actors.actors[0].animation_frame != 0u ||
      world->actors.actors[0].position.x != 91467 ||
      world->actors.actors[0].position.y != 1532) {
    fprintf(stderr, "Ostare did not begin with the retail idle frame\n");
    return 1;
  }
  sf_scenario_actor_update(&world->actors.actors[0], &collision);
  if (world->actors.actors[0].animation_chart != 0u ||
      world->actors.actors[0].animation_frame != 1u) {
    fprintf(stderr, "Remote Town actors do not animate at update cadence\n");
    return 1;
  }
  for (index = 2u; index < 30u; ++index)
    sf_scenario_actor_update(&world->actors.actors[0], &collision);
  if (world->actors.actors[0].animation_chart != 0u ||
      world->actors.actors[0].position.x != 91467 ||
      world->actors.actors[0].position.y != 1532) {
    fprintf(stderr, "Ostare left before the authored idle duration\n");
    return 1;
  }
  sf_scenario_actor_update(&world->actors.actors[0], &collision);
  if (world->actors.actors[0].animation_chart != 1u ||
      (world->actors.actors[0].position.x == 91467 &&
       world->actors.actors[0].position.y == 1532) ||
      world->actors.actors[0].destination.x < 91030 ||
      world->actors.actors[0].destination.x > 91736 ||
      world->actors.actors[0].destination.y < 1309 ||
      world->actors.actors[0].destination.y > 1763) {
    fprintf(stderr, "Ostare did not begin the authored retail wander\n");
    return 1;
  }
  return 0;
}

static int test_ostare_conversation(
    const SfGameplayAssets *assets, SfWorldState *world) {
  SfScenarioScriptEnvironment environment;
  SfScenarioScriptResult result;
  SfConversationLayout layout;
  SfWorldRenderView view;
  environment = (SfScenarioScriptEnvironment) {
    &assets->scenario, &world->actors, world->player.position,
    world->player.judgement, world->companion_type};
  result = sf_scenario_actor_script_start_status(
    &world->actor_script_state, assets->script,
    0, 12000000, &environment);
  sf_world_render_view(world, 1000u, &view);
  if (result != SF_SCENARIO_SCRIPT_WAITING_FOR_MESSAGE ||
      world->actor_script_state.message_id != 1000000 ||
      world->actor_script_state.message_actor_id != 0 ||
      !world->actors.actors[0].interaction_active ||
      !sf_conversation_layout_build(
        assets, world, &view, 1000u, &layout) ||
      layout.width != 368 || layout.height != 68 ||
      layout.choice_count != 0u ||
      assets->speech_frame.image_count != 5u) {
    fprintf(stderr, "Ostare's opening bubble differs from retail\n");
    return 1;
  }
  result = sf_scenario_actor_script_resume(
    &world->actor_script_state, assets->script, -1, &environment);
  if (result != SF_SCENARIO_SCRIPT_WAITING_FOR_MESSAGE ||
      world->actor_script_state.message_id != 1000001) {
    fprintf(stderr, "Ostare's first callback differs from retail\n");
    return 1;
  }
  result = sf_scenario_actor_script_resume(
    &world->actor_script_state, assets->script, -1, &environment);
  if (result != SF_SCENARIO_SCRIPT_WAITING_FOR_MESSAGE ||
      world->actor_script_state.message_id != 1000002) {
    fprintf(stderr, "Ostare's second callback differs from retail\n");
    return 1;
  }
  return 0;
}

static int test_actor_pointer(
    const SfGameplayAssets *assets, SfWorldState *world) {
  const SfScenarioActor *actor = &world->actors.actors[0];
  SfWorldRenderView view;
  SfScreenPoint anchor;
  SfGameInput input;
  int exact_x = -1;
  int exact_y = -1;
  int y;
  sf_world_render_view(world, 1000u, &view);
  anchor = sf_world_to_screen(actor->position);
  anchor.x -= view.camera_x;
  anchor.y -= view.camera_y;
  world->pointer.range_enabled = false;
  for (y = anchor.y - 100; y <= anchor.y + 40 && exact_x < 0; ++y) {
    int x;
    for (x = anchor.x - 80; x <= anchor.x + 80; ++x) {
      memset(&input, 0, sizeof(input));
      input.pointer_active = true;
      input.pointer_x = (int16_t) x;
      input.pointer_y = (int16_t) y;
      sf_world_pointer_resolve(assets, world, &input);
      if (input.pointed_actor_id == actor->id) {
        exact_x = x;
        exact_y = y;
        break;
      }
    }
  }
  if (exact_x < 0) {
    fprintf(stderr, "Ostare has no exact opaque pointer cell\n");
    return 1;
  }
  for (y = exact_y - 16; y <= exact_y + 16; ++y) {
    int x;
    for (x = exact_x - 16; x <= exact_x + 16; ++x) {
      memset(&input, 0, sizeof(input));
      input.pointer_active = true;
      input.pointer_x = (int16_t) x;
      input.pointer_y = (int16_t) y;
      sf_world_pointer_resolve(assets, world, &input);
      if (input.pointed_actor_id >= 0) continue;
      world->pointer.range_enabled = true;
      sf_world_pointer_resolve(assets, world, &input);
      if (input.pointed_actor_id == actor->id) return 0;
      world->pointer.range_enabled = false;
    }
  }
  fprintf(stderr, "Ostare was not selectable through the retail range square\n");
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
  if (test_scenario_actors(&assets, &world)) return 1;
  sf_world_state_enter(
    &world, assets.entry.world_x, assets.entry.world_y,
    (uint8_t) assets.entry.direction);
  if (test_actor_pointer(&assets, &world)) return 1;
  if (test_ostare_conversation(&assets, &world)) return 1;
  world.pointer.range_enabled = true;
  if (!sf_gameplay_screen_init(&screen, &assets, &world) ||
      screen.scene.visible_count != 22u || screen.scene.shadow_count != 10u) {
    fprintf(stderr,
      "Remote Town viewport culling changed before retail depth sorting\n");
    return 1;
  }
#endif
  return 0;
}
