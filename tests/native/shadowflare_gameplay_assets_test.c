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
#include "game/world_conversation.h"
#include "game/world_script.h"
#include "screens/gameplay_screen.h"
#include "screens/gameplay_object_visual.h"
#include "ui/conversation_input.h"
#include "ui/conversation_layout.h"
#include "ui/gameplay_hud.h"
#include "ui/gameplay_inventory.h"
#include "ui/gameplay_inventory_input.h"
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
static uint16_t sf_gameplay_test_pixels[640u * 480u];
static uint16_t sf_gameplay_test_hud_copy[640u * 87u];

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

static int test_ground_item_assets(const SfGameplayAssets *assets) {
  const SfGroundItemVisual *visual = sf_ground_item_visual(
    &assets->ground_items, 0);
  static const int32_t artwork_patterns[4] = {77, 82, 113, 107};
  static const int32_t shadow_patterns[4] = {0, 5, 36, 30};
  static const int32_t palettes[4] = {0, 10, 72, 60};
  static const int32_t charts[4] = {0, 5, 36, 30};
  uint8_t index;
  if (assets->ground_items.definition_count != 8u ||
      assets->ground_items.visual_count != 1u || !visual ||
      !sf_ground_item_sound(&assets->ground_items, 15u) ||
      !sf_ground_item_sound(&assets->ground_items, 48u)) {
    fprintf(stderr, "Remote Town ground-item resources differ from retail\n");
    return 1;
  }
  for (index = 0u; index < 4u; ++index) {
    if (!sf_ground_item_animation(visual, charts[index]) ||
        !sf_njp_sparse_pattern(&visual->artwork, artwork_patterns[index]) ||
        !sf_njp_sparse_pattern(&visual->shadows, shadow_patterns[index]) ||
        !sf_njp_sparse_palette(&visual->artwork, palettes[index])) {
      fprintf(stderr, "A starter drop is missing its retail cells or palette\n");
      return 1;
    }
  }
  {
    static const uint8_t expected_categories[4] = {0u, 1u, 0u, 4u};
    static const int32_t expected_ids[4] = {0, 1000000, 100, 0};
    static const char *expected_names[4] = {
      "Short Sword", "Round Shield", "Dagger", "Gold"
    };
    uint8_t expected;
    for (expected = 0u; expected < 4u; ++expected) {
      const SfItemGroundDefinition *definition = NULL;
      for (index = 0u; index < assets->ground_items.definition_count; ++index) {
        const SfItemGroundDefinition *candidate =
          &assets->ground_items.definitions[index];
        if (candidate->category == expected_categories[expected] &&
            candidate->definition_id == expected_ids[expected]) {
          definition = candidate;
          break;
        }
      }
      if (!definition || strcmp(
            definition->name, expected_names[expected]) != 0) {
        fprintf(stderr, "A starter drop has the wrong retail item name\n");
        return 1;
      }
    }
  }
  return 0;
}

static int test_gameplay_hud(
    const SfGameplayAssets *assets, SfPlayerState *player) {
  SfRenderer renderer;
  size_t index;
  size_t changed = 0u;
  if (assets->hud.pattern_count != 18u ||
      assets->player_parameters.values[0] != 100 ||
      assets->player_parameters.values[1] != 128 ||
      assets->player_parameters.values[2] != 150 ||
      assets->player_parameters.values[3] != 150 ||
      assets->player_parameters.experience_threshold != 25 ||
      !sf_player_apply_initial_parameters(
        player, &assets->player_parameters) ||
      player->current_life != 150 || player->current_mana != 150 ||
      player->walking_speed_tier != 5u) {
    fprintf(stderr, "The retail new-player HUD values differ\n");
    return 1;
  }
  if (!sf_renderer_init(
        &renderer, sf_gameplay_test_pixels,
        sizeof(sf_gameplay_test_pixels), 640u, 480u)) return 1;
  sf_renderer_clear(&renderer, 0x1234u);
  sf_gameplay_hud_draw(&renderer, assets, player, NULL);
  for (index = 0u; index < 640u * 393u; ++index) {
    if (sf_gameplay_test_pixels[index] != 0x1234u) {
      fprintf(stderr, "The gameplay HUD drew above its retail surface\n");
      return 1;
    }
  }
  for (; index < 640u * 480u; ++index)
    if (sf_gameplay_test_pixels[index] != 0x1234u) ++changed;
  if (changed < 640u * 60u) {
    fprintf(stderr, "The retail bottom HUD was not composed\n");
    return 1;
  }
  memcpy(
    sf_gameplay_test_hud_copy,
    sf_gameplay_test_pixels + 640u * 393u,
    sizeof(sf_gameplay_test_hud_copy));
  player->pace = SF_PLAYER_PACE_RUN;
  sf_renderer_clear(&renderer, 0x1234u);
  sf_gameplay_hud_draw(&renderer, assets, player, NULL);
  if (memcmp(
        sf_gameplay_test_hud_copy,
        sf_gameplay_test_pixels + 640u * 393u,
        sizeof(sf_gameplay_test_hud_copy)) == 0) {
    fprintf(stderr, "Walk and run use the same HUD indicator\n");
    return 1;
  }
  player->pace = SF_PLAYER_PACE_WALK;
  return 0;
}

static const SfItemGroundDefinition *find_ground_definition(
    const SfGameplayAssets *assets, uint8_t category, int32_t definition_id) {
  uint8_t index;
  for (index = 0u; index < assets->ground_items.definition_count; ++index) {
    const SfItemGroundDefinition *definition =
      &assets->ground_items.definitions[index];
    if (definition->category == category &&
        definition->definition_id == definition_id) return definition;
  }
  return NULL;
}

static int test_inventory_storage(const SfGameplayAssets *assets) {
  const SfItemGroundDefinition *dagger =
    find_ground_definition(assets, 0u, 100);
  const SfItemGroundDefinition *gold =
    find_ground_definition(assets, 4u, 0);
  SfInventoryState inventory;
  SfInventoryState full;
  uint8_t index;
  if (!dagger || !gold) return 1;
  sf_inventory_init(&inventory);
  if (!sf_inventory_store(&inventory, gold, 15000) ||
      !sf_inventory_store(&inventory, gold, 5001) ||
      inventory.count != 3u ||
      inventory.items[0].quantity != 10000 ||
      inventory.items[1].quantity != 10000 ||
      inventory.items[2].quantity != 1 ||
      sf_inventory_gold(&inventory) != 20001) {
    fprintf(stderr, "Retail gold stacks are not stored transactionally\n");
    return 1;
  }
  sf_inventory_init(&inventory);
  for (index = 0u; index < 9u; ++index) {
    if (!sf_inventory_store(&inventory, dagger, 1)) {
      fprintf(stderr, "A Dagger did not fit in an empty retail column\n");
      return 1;
    }
  }
  full = inventory;
  if (sf_inventory_store(&inventory, dagger, 1) ||
      memcmp(&inventory, &full, sizeof(inventory)) != 0) {
    fprintf(stderr, "A failed pickup partially changed the inventory\n");
    return 1;
  }
  return 0;
}

static int test_gameplay_inventory(
    const SfGameplayAssets *assets, const SfPlayerState *player) {
  const SfItemGroundDefinition *dagger =
    find_ground_definition(assets, 0u, 100);
  const SfNjpSparseResource *artwork;
  SfGameplayInventoryUi inventory;
  SfPlayerState empty_player;
  SfRenderer renderer;
  SfGameInput input;
  uint16_t empty_item[32u * 96u];
  size_t changed = 0u;
  int y;
  if (!dagger || assets->inventory_panel.pattern_count != 6u ||
      !sf_njp_decoded_pattern(&assets->inventory_panel, 2u) ||
      sf_njp_decoded_pattern(
        &assets->inventory_panel, 2u)->reference_count != 3u ||
      !sf_njp_decoded_pattern(&assets->inventory_panel, 116u) ||
      !sf_njp_decoded_pattern(&assets->inventory_panel, 117u) ||
      sf_njp_decoded_pattern(&assets->inventory_panel, 74u)) {
    fprintf(stderr, "The retail inventory panel patterns are incomplete\n");
    return 1;
  }
  artwork = sf_inventory_item_artwork(
    &assets->inventory_items, dagger->inventory_pattern_group);
  if (!artwork || !sf_njp_sparse_pattern(
        artwork, dagger->inventory_pattern)) {
    fprintf(stderr, "The Dagger inventory artwork was not retained\n");
    return 1;
  }
  sf_gameplay_inventory_init(&inventory);
  memset(&input, 0, sizeof(input));
  input.inventory_pressed = true;
  if (!sf_gameplay_inventory_input_resolve(
        &inventory, player, false, &input) || !inventory.open ||
      input.world_view_offset_x != SF_GAMEPLAY_INVENTORY_VIEW_OFFSET) {
    fprintf(stderr, "The inventory key did not open the retail panel\n");
    return 1;
  }
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_x = 500;
  input.pointer_y = 100;
  (void) sf_gameplay_inventory_input_resolve(
    &inventory, player, false, &input);
  if (!input.pointer_over_gameplay_ui ||
      input.world_view_offset_x != SF_GAMEPLAY_INVENTORY_VIEW_OFFSET) {
    fprintf(stderr, "The open inventory leaked its pointer into the world\n");
    return 1;
  }
  if (!sf_renderer_init(
        &renderer, sf_gameplay_test_pixels,
        sizeof(sf_gameplay_test_pixels), 640u, 480u)) return 1;
  empty_player = *player;
  sf_inventory_init(&empty_player.inventory);
  sf_renderer_clear(&renderer, 0x1234u);
  sf_gameplay_inventory_draw(
    &renderer, assets, &empty_player, &inventory, NULL);
  for (y = 0; y < 96; ++y)
    memcpy(
      empty_item + (size_t) y * 32u,
      sf_gameplay_test_pixels + (size_t) (264 + y) * 640u + 336u,
      32u * sizeof(uint16_t));
  sf_renderer_clear(&renderer, 0x1234u);
  sf_gameplay_inventory_draw(
    &renderer, assets, player, &inventory, NULL);
  for (y = 0; y < 96; ++y) {
    int x;
    for (x = 0; x < 32; ++x) {
      if (sf_gameplay_test_pixels[
            (size_t) (264 + y) * 640u + 336u + x] !=
          empty_item[(size_t) y * 32u + x]) ++changed;
    }
  }
  if (changed < 10u) {
    fprintf(stderr, "The picked-up Dagger was not drawn in its backpack cells\n");
    return 1;
  }
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_primary_pressed = true;
  input.pointer_x = 380;
  input.pointer_y = 398;
  if (!sf_gameplay_inventory_input_resolve(
        &inventory, player, false, &input) || inventory.open ||
      !input.pointer_over_gameplay_ui || input.world_view_offset_x != 0) {
    fprintf(stderr, "The authored inventory close control did not close\n");
    return 1;
  }
  return 0;
}

static int test_gameplay_ui_conversation_guard(SfWorldState *world) {
  SfWorldState guarded = *world;
  SfGameInput input;
  guarded.actor_script_state.message_active = true;
  guarded.actor_script_state.message_selection_pending = false;
  memset(&input, 0, sizeof(input));
  input.pointer_primary_pressed = true;
  input.pointer_over_gameplay_ui = true;
  if (!sf_world_conversation_update(&guarded, &input) ||
      !guarded.actor_script_state.message_active) {
    fprintf(stderr, "A panel click advanced the conversation behind it\n");
    return 1;
  }
  return 0;
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
  const SfWorldPoint interaction_position = world->actors.actors[0].position;
  environment = sf_world_script_environment(world);
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
  result = sf_scenario_actor_script_resume(
    &world->actor_script_state, assets->script, -1, &environment);
  if (result != SF_SCENARIO_SCRIPT_WAITING_FOR_MESSAGE ||
      world->actor_script_state.message_id != 1000003 ||
      world->ground_items.count != 4u ||
      world->ground_items.items[0].category != 0u ||
      world->ground_items.items[0].definition_id != 0 ||
      world->ground_items.items[0].quantity != 1 ||
      world->ground_items.items[0].position.x != interaction_position.x + 200 ||
      world->ground_items.items[0].position.y != interaction_position.y ||
      world->ground_items.items[0].animation_chart != 0 ||
      world->ground_items.items[1].category != 1u ||
      world->ground_items.items[1].definition_id != 1000000 ||
      world->ground_items.items[1].position.x != interaction_position.x ||
      world->ground_items.items[1].position.y != interaction_position.y + 200 ||
      world->ground_items.items[1].animation_chart != 5 ||
      world->ground_items.items[1].red_strength != 900 ||
      world->ground_items.items[1].green_strength != 800 ||
      world->ground_items.items[1].blue_strength != 500 ||
      world->ground_items.items[2].category != 0u ||
      world->ground_items.items[2].definition_id != 100 ||
      world->ground_items.items[2].position.x != interaction_position.x + 200 ||
      world->ground_items.items[2].position.y != interaction_position.y - 200 ||
      world->ground_items.items[2].animation_chart != 36 ||
      world->ground_items.items[3].category != 4u ||
      world->ground_items.items[3].definition_id != 0 ||
      world->ground_items.items[3].quantity != 200 ||
      world->ground_items.items[3].position.x != interaction_position.x + 200 ||
      world->ground_items.items[3].position.y != interaction_position.y ||
      world->ground_items.items[3].animation_chart != 30) {
    fprintf(stderr, "Ostare's retail starter drops differ\n");
    return 1;
  }
  {
    uint8_t ordinary_sounds = 0u;
    uint8_t gold_sounds = 0u;
    uint8_t update;
    for (update = 0u; update < 19u; ++update) {
      sf_ground_items_update(&world->ground_items);
      {
        uint8_t sound;
        for (sound = 0u; sound < world->ground_items.sound_count; ++sound) {
          if (world->ground_items.sound_samples[sound] == 15u)
            ++ordinary_sounds;
          if (world->ground_items.sound_samples[sound] == 85u)
            ++gold_sounds;
        }
      }
    }
    if (ordinary_sounds != 3u || gold_sounds != 1u ||
        world->ground_items.items[0].bounce_state != 2u ||
        world->ground_items.items[3].bounce_state != 2u) {
      fprintf(stderr, "Ostare's drops do not use the retail bounce sounds\n");
      return 1;
    }
  }
  return 0;
}

static int test_harley_conversation(
    const SfGameplayAssets *assets, SfWorldState *world) {
  SfScenarioScriptEnvironment environment;
  SfScenarioScriptResult result;
  SfConversationLayout layout;
  SfWorldRenderView view;
  SfScreenPoint actor_screen;
  SfWorldPoint destination;
  SfGameInput input;
  const SfConversationChoice *choice;
  sf_scenario_actor_release_interaction(&world->actors.actors[0]);
  environment = sf_world_script_environment(world);
  result = sf_scenario_actor_script_start_status(
    &world->actor_script_state, assets->script,
    0, sf_scenario_actor_character_number(&world->actors.actors[6]),
    &environment);
  actor_screen = sf_world_to_screen(world->actors.actors[6].position);
  world->camera_x = actor_screen.x - 320;
  world->camera_y = actor_screen.y - 240;
  sf_world_render_view(world, 1000u, &view);
  if (result != SF_SCENARIO_SCRIPT_WAITING_FOR_MESSAGE ||
      world->actor_script_state.message_id != 1000056 ||
      !world->actor_script_state.message_selection_pending ||
      !sf_conversation_layout_build(
        assets, world, &view, 1000u, &layout) ||
      layout.choice_count != 4u || strchr(layout.text, '~')) {
    fprintf(stderr,
      "Harley's authored choice menu differs from retail "
      "(result=%d id=%d pending=%d choices=%u text=%s)\n",
      (int) result, (int) world->actor_script_state.message_id,
      world->actor_script_state.message_selection_pending ? 1 : 0,
      (unsigned) layout.choice_count, layout.text);
    return 1;
  }
  choice = &layout.choices[1];
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_primary_pressed = true;
  input.pointer_x = (int16_t) (
    layout.x + 4 + choice->column * layout.cell_width + 1);
  input.pointer_y = (int16_t) (
    layout.y + 4 + choice->line * layout.cell_height + 1);
  sf_conversation_input_resolve(assets, world, &input);
  destination = world->player.destination;
  sf_world_state_update(world, &input);
  if (!input.conversation_choices_resolved ||
      input.conversation_option_count != 4u ||
      input.pointed_conversation_option != 1 ||
      world->actor_script_state.message_id != 1000057 ||
      world->actor_script_state.message_selection_pending ||
      world->player.destination.x != destination.x ||
      world->player.destination.y != destination.y) {
    fprintf(stderr, "Harley's Explanation choice did not consume its click\n");
    return 1;
  }
  memset(&input, 0, sizeof(input));
  input.pointer_primary_pressed = true;
  sf_world_state_update(world, &input);
  if (world->actor_script_state.message_id != 1000058 ||
      world->actor_script_state.message_selection_pending) {
    fprintf(stderr, "Harley's explanation did not reach its second line\n");
    return 1;
  }
  sf_world_state_update(world, &input);
  if (world->actor_script_state.message_active ||
      world->actors.actors[6].interaction_active) {
    fprintf(stderr, "Harley was not released after his explanation\n");
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
  if (exact_x < SF_GAMEPLAY_INVENTORY_VIEW_OFFSET) {
    fprintf(stderr, "Ostare's exact cell cannot exercise the shifted view\n");
    return 1;
  }
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_x = (int16_t) (
    exact_x - SF_GAMEPLAY_INVENTORY_VIEW_OFFSET);
  input.pointer_y = (int16_t) exact_y;
  input.world_view_offset_x = SF_GAMEPLAY_INVENTORY_VIEW_OFFSET;
  sf_world_pointer_resolve(assets, world, &input);
  if (input.pointed_actor_id != actor->id) {
    fprintf(stderr, "World picking did not follow the inventory camera shift\n");
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

static int test_ground_item_pickup(
    const SfGameplayAssets *assets, SfWorldState *world) {
  const int32_t item_id = world->ground_items.items[2].id;
  const SfWorldPoint item_position = world->ground_items.items[2].position;
  SfScreenPoint anchor = sf_world_to_screen(item_position);
  SfGameInput input;
  int pointer_x = -1;
  int pointer_y = -1;
  int y;
  world->camera_x = anchor.x - 320;
  world->camera_y = anchor.y - 240;
  for (y = 140; y < 340 && pointer_x < 0; ++y) {
    int x;
    for (x = 220; x < 420; ++x) {
      memset(&input, 0, sizeof(input));
      input.pointer_active = true;
      input.pointer_x = (int16_t) x;
      input.pointer_y = (int16_t) y;
      sf_world_pointer_resolve(assets, world, &input);
      if (input.pointed_ground_item_id == item_id) {
        pointer_x = x;
        pointer_y = y;
        break;
      }
    }
  }
  if (pointer_x < 0) {
    fprintf(stderr, "The Dagger has no retail pointer hit\n");
    return 1;
  }
  memset(&input, 0, sizeof(input));
  input.pointer_active = true;
  input.pointer_x = (int16_t) pointer_x;
  input.pointer_y = (int16_t) pointer_y;
  input.pointer_primary_pressed = true;
  input.world_pointer_resolved = true;
  input.pointed_actor_id = -1;
  input.pointed_ground_item_id = item_id;
  sf_world_state_update(world, &input);
  memset(&input, 0, sizeof(input));
  input.pointed_actor_id = -1;
  input.pointed_ground_item_id = -1;
  {
    uint16_t update;
    for (update = 0u; update < 2000u &&
         world->ground_items.count == 4u; ++update)
      sf_world_state_update(world, &input);
  }
  if (world->ground_items.count != 3u ||
      world->player.inventory.count != 1u ||
      world->player.inventory.items[0].category != 0u ||
      world->player.inventory.items[0].definition_id != 100 ||
      world->player.inventory.items[0].width != 1u ||
      world->player.inventory.items[0].height != 3u ||
      world->player.inventory.items[0].durability != 300 ||
      world->ground_items.sound_count != 1u ||
      world->ground_items.sound_samples[0] != 48u) {
    fprintf(stderr, "The retail Dagger approach and pickup differs\n");
    return 1;
  }
  return 0;
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
  if (test_ground_item_assets(&assets)) return 1;
  if (test_gameplay_hud(&assets, &player)) return 1;
  if (test_inventory_storage(&assets)) return 1;
  sf_world_state_init(&world, 0, 0, player.gender);
  if (!sf_player_apply_initial_parameters(
        &world.player, &assets.player_parameters)) return 1;
  sf_world_state_bind_collision(&world, &assets.ground, &assets.objects);
  sf_world_state_bind_ground_items(
    &world, assets.ground_items.definitions,
    assets.ground_items.definition_count);
  if (test_scenario_actors(&assets, &world)) return 1;
  sf_world_state_enter(
    &world, assets.entry.world_x, assets.entry.world_y,
    (uint8_t) assets.entry.direction);
  if (test_actor_pointer(&assets, &world)) return 1;
  if (test_ostare_conversation(&assets, &world)) return 1;
  if (test_harley_conversation(&assets, &world)) return 1;
  world.pointer.range_enabled = true;
  if (!sf_gameplay_screen_init(&screen, &assets, &world) ||
      screen.scene.visible_count != 22u || screen.scene.shadow_count != 10u) {
    fprintf(stderr,
      "Remote Town viewport culling changed before retail depth sorting\n");
    return 1;
  }
  if (test_ground_item_pickup(&assets, &world)) return 1;
  if (test_gameplay_inventory(&assets, &world.player)) return 1;
  if (test_gameplay_ui_conversation_guard(&world)) return 1;
#endif
  return 0;
}
