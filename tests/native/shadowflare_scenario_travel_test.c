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

#include "core/memory_budget.h"
#include "data/save.h"
#include "game/game.h"
#include "game/scenario_object.h"
#include "game/world_transport.h"
#include "runtime/screen_runtime.h"

#include <stdio.h>
#include <string.h>

typedef union SfScenarioTravelTestMemory {
  long double alignment;
  void *pointer;
  uint8_t bytes[SF_MAIN_ARENA_BYTES];
} SfScenarioTravelTestMemory;

static SfScenarioTravelTestMemory sf_scenario_travel_memory;
static uint8_t sf_scenario_travel_video_memory[SF_VIDEO_MEMORY_LIMIT_BYTES];

static void sf_scenario_travel_test_input(SfGameInput *input) {
  memset(input, 0, sizeof(*input));
  input->pointed_actor_id = -1;
  input->pointed_enemy_id = -1;
  input->pointed_scenario_object_id = -1;
  input->pointed_ground_item_id = -1;
  input->transport_destination = -1;
  input->belt_pocket_pressed = -1;
}

static int sf_scenario_travel_test_enter_trigger(
    SfGame *game, int32_t character_number,
    int32_t scenario_id, int32_t entry_value) {
  SfScenarioObject *trigger = sf_scenario_object_find(
    &game->world.scenario_objects, character_number);
  SfGameInput input;
  if (!trigger) {
    fprintf(stderr, "The authored scenario trigger is missing\n");
    return 1;
  }
  sf_world_state_enter(
    &game->world, trigger->position.x, trigger->position.y, 7u);
  sf_scenario_travel_test_input(&input);
  sf_world_state_update(&game->world, &input);
  if (!game->world.travel_request.pending ||
      game->world.travel_request.scenario_id != scenario_id ||
      game->world.travel_request.entry_value != entry_value) {
    fprintf(stderr, "Status kind three did not publish opcode 17 travel\n");
    return 1;
  }
  return 0;
}

int main(void) {
#if defined(OPENSHADOWFLARE_SOURCE_DIR)
  SfArena arena;
  SfArena video_arena;
  SfGame *game;
  SfScreenRuntime *runtime;
  void *scratch;
  SfGameInput input;
  SfWorldPoint town_entry;
  uint8_t town_direction;
  bool companion_inactive;
  bool save_zero_available;
  SfSaveCatalog save_catalog;
  char root[1024];
  char probe_path[2048];
  FILE *probe;
  (void) snprintf(
    root, sizeof(root), "%s/tmp/ShadowFlare", OPENSHADOWFLARE_SOURCE_DIR);
  (void) snprintf(
    probe_path, sizeof(probe_path),
    "%s/Scenario/00000001/Scenario.Mct", root);
  probe = fopen(probe_path, "rb");
  if (!probe) return 0;
  fclose(probe);
  (void) snprintf(probe_path, sizeof(probe_path), "%s/Save/0000.Ssv", root);
  probe = fopen(probe_path, "rb");
  save_zero_available = probe != NULL;
  if (probe) fclose(probe);
  sf_arena_init(
    &arena, sf_scenario_travel_memory.bytes,
    sizeof(sf_scenario_travel_memory.bytes));
  sf_arena_init(
    &video_arena, sf_scenario_travel_video_memory,
    sizeof(sf_scenario_travel_video_memory));
  if (!sf_arena_push(
        &video_arena, SF_FRAMEBUFFER_BYTES, sizeof(uint16_t))) return 1;
  game = (SfGame *) sf_arena_push_zero(
    &arena, sizeof(*game), sizeof(void *));
  runtime = (SfScreenRuntime *) sf_arena_push_zero(
    &arena, sizeof(*runtime), sizeof(void *));
  scratch = sf_arena_push(&arena, 60000u, 4u);
  if (!game || !runtime || !scratch ||
      !sf_screen_runtime_init(
        runtime, &arena, &video_arena, root, scratch, 60000u)) return 1;
  sf_game_init(game, NULL);
  sf_world_state_init(&game->world, 0, 0, 1u);
  game->mode = SF_GAME_MODE_GAMEPLAY;
  if (!sf_screen_runtime_load(runtime, game)) {
    fprintf(stderr, "Remote Town did not load through the screen runtime\n");
    return 1;
  }
  town_entry = game->world.player.position;
  town_direction = game->world.player.direction;
  game->world.actor_script_state.progress.persistent_values[17] = 42;
  game->world.actor_script_state.progress.persistent_count = 18u;
  game->world.actor_script_state.progress.transport_values[1] = 1;
  game->world.actor_script_state.progress.transport_count = 2u;
  companion_inactive = game->world.companion.inactive;
  game->world.ground_items.count = 1u;
  if (sf_scenario_travel_test_enter_trigger(
        game, 10000000, 1, 0) ||
      !sf_screen_runtime_prepare(runtime, game)) return 1;
  if (game->world.scenario_id != 1 || game->world.entry_key != 0 ||
      game->world.player.position.x != 90581 ||
      game->world.player.position.y != 5288 ||
      game->world.player.direction != 7u ||
      game->world.actor_script_state.progress.persistent_count != 18u ||
      game->world.actor_script_state.progress.persistent_values[17] != 42 ||
      game->world.actor_script_state.progress.transport_count != 2u ||
      game->world.actor_script_state.progress.transport_values[1] != 1 ||
      game->world.ground_items.count != 0u ||
      game->world.companion.inactive != companion_inactive ||
      game->world.travel_request.pending ||
      runtime->assets.gameplay.scenario.object_count != 48u ||
      runtime->assets.gameplay.scenario.people_count != 0u ||
      runtime->assets.gameplay.scenario.enemy_count != 127u ||
      game->world.enemies.count != 127u) {
    fprintf(stderr, "The Remote Town exit did not preserve the live owners\n");
    return 1;
  }
  sf_scenario_travel_test_input(&input);
  sf_world_state_update(&game->world, &input);
  if (game->world.travel_request.pending) {
    fprintf(stderr, "The outdoor entry immediately retriggered travel\n");
    return 1;
  }
  {
    SfScenarioEnemy *goblin = NULL;
    const SfAiAction *direct_attack = NULL;
    bool retreat_action_seen = false;
    uint16_t enemy_index;
    uint16_t action_index;
    bool presentation_seen = false;
    bool impact_seen = false;
    bool completion_seen = false;
    uint8_t update;
    for (action_index = 0u;
         action_index < game->world.ai_controls->action_count;
         ++action_index) {
      if (game->world.ai_controls->actions[action_index].action_number == 9) {
        retreat_action_seen = true;
        break;
      }
    }
    if (!retreat_action_seen) {
      fprintf(stderr, "Near Remote Town lost its retail retreat action\n");
      return 1;
    }
    for (enemy_index = 0u; enemy_index < game->world.enemies.count;
         ++enemy_index) {
      SfScenarioEnemy *candidate =
        &game->world.enemies.enemies[enemy_index];
      if (candidate->definition && candidate->definition->id == 101) {
        goblin = candidate;
        break;
      }
    }
    if (goblin && goblin->control) {
      for (action_index = 0u;
           action_index < goblin->control->events[0].action_count;
           ++action_index) {
        const SfAiAction *candidate = sf_ai_control_action(
          game->world.ai_controls, goblin->control, 0u, action_index);
        if (candidate && candidate->action_number == 2) {
          direct_attack = candidate;
          break;
        }
      }
    }
    if (!goblin || !direct_attack) {
      fprintf(stderr, "The gate Goblin has no retail direct attack\n");
      return 1;
    }
    goblin->position.x = game->world.player.position.x + 100;
    goblin->position.y = game->world.player.position.y;
    goblin->previous_position = goblin->position;
    goblin->selected_action = direct_attack;
    goblin->current_action = -1;
    goblin->event_number = -1;
    goblin->presentation_action = 7u;
    sf_world_state_update(&game->world, &input);
    if (game->world.enemy_attack_request.resource_id !=
          goblin->definition->resource_id ||
        game->world.enemy_attack_request.chart != 4 ||
        !sf_screen_runtime_prepare(runtime, game) ||
        !runtime->assets.gameplay.enemies.attack.loaded ||
        runtime->assets.gameplay.enemies.attack.resource_id !=
          goblin->definition->resource_id ||
        !goblin->direct_attack_animations ||
        video_arena.used > video_arena.capacity) {
      fprintf(stderr, "The enemy attack working set did not load on demand\n");
      return 1;
    }
    for (update = 0u; update < 64u && !completion_seen; ++update) {
      sf_world_state_update(&game->world, &input);
      if (goblin->presentation_action == 1u) presentation_seen = true;
      if (goblin->direct_impact_pending) impact_seen = true;
      if (presentation_seen && goblin->event_number == 2)
        completion_seen = true;
    }
    if (!presentation_seen || !impact_seen || !completion_seen) {
      fprintf(stderr, "The retail Goblin attack did not cross its CAF marker\n");
      return 1;
    }
  }
  if (sf_scenario_travel_test_enter_trigger(
        game, 10000000, 0, 0) ||
      !sf_screen_runtime_prepare(runtime, game)) return 1;
  if (game->world.scenario_id != 0 || game->world.entry_key != 0 ||
      game->world.player.position.x != town_entry.x ||
      game->world.player.position.y != town_entry.y ||
      game->world.player.direction != town_direction ||
      game->world.actor_script_state.progress.persistent_values[17] != 42 ||
      game->world.actor_script_state.progress.transport_values[1] != 1 ||
      game->world.travel_request.pending || arena.used > arena.capacity) {
    fprintf(stderr, "The authored return to Remote Town did not round-trip\n");
    return 1;
  }
  if (!sf_world_transport_activate(&game->world, 1) ||
      !game->world.travel_request.pending ||
      game->world.travel_request.scenario_id != 6 ||
      game->world.travel_request.entry_value != 4) {
    fprintf(stderr, "Cross-scenario transport bypassed the travel seam\n");
    return 1;
  }
  sf_scenario_travel_clear(&game->world.travel_request);
  if (save_zero_available) {
    game->mode = SF_GAME_MODE_LOAD_GAME;
    if (!sf_screen_runtime_load(runtime, game)) {
      fprintf(stderr, "The load screen could not replace gameplay assets\n");
      return 1;
    }
    sf_world_state_init(&game->world, 0, 0, game->player_gender);
    game->load_game.selected_file_slot = 0;
    game->mode = SF_GAME_MODE_GAMEPLAY;
    if (!sf_screen_runtime_load(runtime, game) ||
        game->load_game.selected_file_slot != -1 ||
        game->world.scenario_id != 1 || !game->world.entered) {
      fprintf(stderr, "A saved non-town world regressed at the runtime seam\n");
      return 1;
    }
  }
  if (!sf_save_catalog_load(root, &save_catalog)) {
    fprintf(stderr, "The save catalog could not be inspected\n");
    return 1;
  }
  {
    uint8_t save_index;
    for (save_index = 0u; save_index < save_catalog.count; ++save_index) {
      game->mode = SF_GAME_MODE_LOAD_GAME;
      if (!sf_screen_runtime_load(runtime, game)) {
        fprintf(stderr, "The load screen could not replace a saved world\n");
        return 1;
      }
      sf_world_state_init(&game->world, 0, 0, game->player_gender);
      game->load_game.selected_file_slot = (int8_t)
        save_catalog.entries[save_index].file_slot;
      game->mode = SF_GAME_MODE_GAMEPLAY;
      if (!sf_screen_runtime_load(runtime, game) ||
          game->load_game.selected_file_slot != -1 ||
          !game->world.entered) {
        fprintf(stderr, "Save catalog entry %u could not enter its world\n",
          (unsigned) save_catalog.entries[save_index].file_slot);
        return 1;
      }
    }
  }
#endif
  return 0;
}
