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

#include "runtime/screen_runtime.h"

#include "ui/conversation_input.h"
#include "ui/gameplay_hud_input.h"
#include "ui/gameplay_inventory_input.h"
#include "ui/world_pointer.h"

#include <string.h>

bool sf_screen_runtime_init(
    SfScreenRuntime *runtime, SfArena *arena, const char *data_root,
    void *decode_scratch, size_t decode_scratch_size) {
  if (!runtime || !arena || !data_root || !decode_scratch ||
      decode_scratch_size == 0u) return false;
  memset(runtime, 0, sizeof(*runtime));
  runtime->arena = arena;
  runtime->data_root = data_root;
  runtime->decode_scratch = decode_scratch;
  runtime->decode_scratch_size = decode_scratch_size;
  runtime->arena_mark = sf_arena_mark(arena);
  return true;
}

bool sf_screen_runtime_load(SfScreenRuntime *runtime, SfGame *game) {
  bool success = true;
  SfGameMode mode;
  if (!runtime || !runtime->arena || !game) return false;
  mode = game->mode;
  if (runtime->loaded && runtime->loaded_mode == mode) return true;
  if (!sf_arena_rewind(runtime->arena, runtime->arena_mark)) return false;
  memset(&runtime->assets, 0, sizeof(runtime->assets));
  memset(&runtime->screen, 0, sizeof(runtime->screen));
  if (mode == SF_GAME_MODE_TITLE) {
    success = sf_title_assets_load(
      &runtime->assets.title, runtime->data_root, runtime->arena,
      runtime->decode_scratch, runtime->decode_scratch_size);
    if (success) success = sf_title_screen_init(
      &runtime->screen.title,
      runtime->decode_scratch, runtime->decode_scratch_size,
      runtime->assets.title.decode_scratch_bytes);
  } else if (mode == SF_GAME_MODE_CHARACTER_SELECT) {
    success = sf_character_create_assets_load(
      &runtime->assets.character_create,
      runtime->data_root, runtime->arena);
    if (success)
      sf_character_create_screen_init(&runtime->screen.character_create);
  } else if (mode == SF_GAME_MODE_LOAD_GAME) {
    success = sf_load_game_assets_load(
      &runtime->assets.load_game,
      runtime->data_root, runtime->arena,
      runtime->decode_scratch, runtime->decode_scratch_size);
    if (success) sf_load_game_screen_init(&runtime->screen.load_game);
  } else if (mode == SF_GAME_MODE_GAMEPLAY) {
    success = sf_gameplay_assets_load(
      &runtime->assets.gameplay, runtime->data_root,
      game->world.scenario_id, game->world.entry_key,
      game->world.player.gender, game->world.player.appearance_parts,
      game->world.player.appearance_part_count,
      game->world.player.visible_items, game->world.player.visible_item_count,
      runtime->arena);
    if (success) {
      const SfMctEntry *entry = &runtime->assets.gameplay.entry;
      if (!game->world.player.parameters_initialized)
        success = sf_player_apply_initial_parameters(
          &game->world.player,
          &runtime->assets.gameplay.player_parameters);
      if (success) {
        sf_world_state_bind_collision(
          &game->world, &runtime->assets.gameplay.ground,
          &runtime->assets.gameplay.objects);
        sf_world_state_bind_ground_items(
          &game->world,
          runtime->assets.gameplay.ground_items.definitions,
          runtime->assets.gameplay.ground_items.definition_count);
        success = sf_world_state_bind_scenario(
          &game->world, &runtime->assets.gameplay.scenario,
          runtime->assets.gameplay.script);
      }
      if (success) sf_world_state_enter(
          &game->world, entry->world_x, entry->world_y,
          (uint8_t) entry->direction);
      if (success) success = sf_gameplay_screen_init(
        &runtime->screen.gameplay, &runtime->assets.gameplay, &game->world);
    }
  }
  runtime->loaded = success;
  runtime->loaded_mode = mode;
  runtime->blank_drawn = false;
  return success;
}

bool sf_screen_runtime_prepare(SfScreenRuntime *runtime, SfGame *game) {
  SfLoadGameAssets *assets;
  if (!runtime || !game || !runtime->loaded) return false;
  if (runtime->loaded_mode != SF_GAME_MODE_LOAD_GAME ||
      game->mode != SF_GAME_MODE_LOAD_GAME) return true;
  assets = &runtime->assets.load_game;
  if (game->load_game.delete_request >= 0) {
    const uint8_t index = (uint8_t) game->load_game.delete_request;
    uint8_t file_slots[SF_SAVE_SLOT_COUNT];
    uint8_t genders[SF_SAVE_SLOT_COUNT];
    uint8_t slot;
    game->load_game.delete_request = -1;
    if (sf_load_game_assets_delete(
          assets, runtime->data_root, index,
          runtime->decode_scratch, runtime->decode_scratch_size)) {
      for (slot = 0u; slot < assets->catalog.count; ++slot)
        file_slots[slot] = assets->catalog.entries[slot].file_slot;
      for (slot = 0u; slot < assets->catalog.count; ++slot)
        genders[slot] = assets->catalog.entries[slot].gender == 1 ? 1u : 0u;
      sf_game_saved_catalog_changed(
        game, file_slots, genders, assets->catalog.count);
    }
  }
  if (assets->catalog.count == 0u) return true;
  return sf_load_game_assets_select_preview(
    assets, runtime->data_root, game->load_game.selection,
    runtime->decode_scratch, runtime->decode_scratch_size);
}

void sf_screen_runtime_resolve_input(
    SfScreenRuntime *runtime, const SfGame *game, SfGameInput *input) {
  if (!input) return;
  input->pointer_over_gameplay_ui = false;
  input->world_view_offset_x = 0;
  input->world_pointer_resolved = false;
  input->pointed_actor_id = -1;
  input->pointed_ground_item_id = -1;
  input->conversation_choices_resolved = false;
  input->pointed_conversation_option = -1;
  input->conversation_option_count = 0u;
  input->inventory_action = SF_INVENTORY_ACTION_NONE;
  input->inventory_item_index = -1;
  input->inventory_grid_x = -1;
  input->inventory_grid_y = -1;
  input->equipment_slot = -1;
  if (!runtime || !runtime->loaded || !game ||
      runtime->loaded_mode != SF_GAME_MODE_GAMEPLAY ||
      game->mode != SF_GAME_MODE_GAMEPLAY) return;
  if (sf_gameplay_inventory_input_resolve(
        &runtime->screen.gameplay.inventory,
        &game->world.player,
        game->world.actor_script_state.message_active, input))
    runtime->screen.gameplay.drawn = false;
  if (game->world.actor_script_state.message_active &&
      !input->pointer_over_gameplay_ui)
    sf_conversation_input_resolve(
      &runtime->assets.gameplay, &game->world, input);
  else if (!game->world.actor_script_state.message_active) {
    if (input->pointer_over_gameplay_ui) return;
    sf_world_pointer_resolve(
      &runtime->assets.gameplay, &game->world, input);
  }
}

const SfTitleAssets *sf_screen_runtime_title_assets(
    const SfScreenRuntime *runtime) {
  if (!runtime || !runtime->loaded ||
      runtime->loaded_mode != SF_GAME_MODE_TITLE) return NULL;
  return &runtime->assets.title;
}

const SfGameplayAssets *sf_screen_runtime_gameplay_assets(
    const SfScreenRuntime *runtime) {
  if (!runtime || !runtime->loaded ||
      runtime->loaded_mode != SF_GAME_MODE_GAMEPLAY) return NULL;
  return &runtime->assets.gameplay;
}

void sf_screen_runtime_draw(
    SfScreenRuntime *runtime, SfRenderer *renderer, const SfGame *game,
    uint16_t interpolation) {
  if (!runtime || !renderer || !game || !runtime->loaded) return;
  if (game->mode == SF_GAME_MODE_TITLE) {
    sf_title_screen_draw(
      &runtime->screen.title, renderer, &runtime->assets.title, game);
  } else if (game->mode == SF_GAME_MODE_CHARACTER_SELECT) {
    sf_character_create_screen_draw(
      &runtime->screen.character_create, renderer,
      &runtime->assets.character_create, game);
  } else if (game->mode == SF_GAME_MODE_LOAD_GAME) {
    sf_load_game_screen_draw(
      &runtime->screen.load_game, renderer,
      &runtime->assets.load_game, game);
  } else if (game->mode == SF_GAME_MODE_GAMEPLAY) {
    sf_gameplay_screen_draw(
      &runtime->screen.gameplay, renderer,
      &runtime->assets.gameplay, game, interpolation);
  } else if (!runtime->blank_drawn) {
    sf_renderer_clear(renderer, 0u);
    runtime->blank_drawn = true;
  }
}
