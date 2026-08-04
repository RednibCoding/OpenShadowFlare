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

#include "runtime/gameplay_runtime.h"

#include "game/player_save.h"
#include "game/world_save.h"

#include <limits.h>

static bool sf_gameplay_runtime_required_items(
    SfGame *game, const SfSavedGame *saved_game,
    SfItemReference *items, uint8_t *item_count) {
  if (!game || !items || !item_count) return false;
  if (saved_game)
    return sf_saved_player_required_items(
      &saved_game->player, items,
      SF_GROUND_ITEM_DEFINITION_LIMIT, item_count);
  return sf_player_required_item_definitions(
    &game->world.player, items,
    SF_GROUND_ITEM_DEFINITION_LIMIT, item_count);
}

bool sf_gameplay_runtime_load(
    SfGameplayAssets *assets, SfGameplayScreen *screen,
    SfArena *arena, const char *data_root, SfGame *game,
    const SfSavedGame *saved_game,
    const SfScenarioTravelRequest *travel) {
  SfItemReference retained_items[SF_GROUND_ITEM_DEFINITION_LIMIT];
  uint8_t retained_item_count = 0u;
  int32_t scenario_id;
  int32_t entry_key;
  int32_t player_level;
  int32_t companion_type;
  int32_t companion_level;
  const SfMctEntry *entry;
  bool success;
  if (!assets || !screen || !arena || !data_root || !game ||
      (saved_game && travel)) return false;
  if (saved_game && !sf_world_prepare_save_load(
        &game->world, saved_game)) return false;
  if (saved_game)
    game->world.player.gender = saved_game->player.gender == 1 ? 1u : 0u;
  scenario_id = travel ? travel->scenario_id : game->world.scenario_id;
  if (travel) {
    if (!travel->pending || travel->entry_value < 0 ||
        travel->entry_value > INT32_MAX / 4) return false;
    entry_key = travel->entry_value * 4;
  } else {
    entry_key = game->world.entry_key;
  }
  player_level = saved_game
    ? saved_game->player.level : game->world.player.level;
  companion_type = saved_game
    ? saved_game->player.companion_type : game->world.player.companions.type;
  companion_level = saved_game
    ? saved_game->player.companion_level :
      sf_player_companion_level(&game->world.player.companions);
  if (!sf_gameplay_runtime_required_items(
        game, saved_game, retained_items, &retained_item_count)) return false;
  success = sf_gameplay_assets_load(
    assets, data_root, scenario_id, entry_key,
    game->world.player.gender, player_level,
    companion_type, companion_level,
    game->world.player.appearance_parts,
    game->world.player.appearance_part_count,
    game->world.player.visible_items,
    game->world.player.visible_item_count,
    retained_items, retained_item_count, arena);
  if (!success) return false;
  entry = &assets->entry;
  if (saved_game)
    success = sf_player_restore_save(
      &game->world.player, &saved_game->player,
      assets->ground_items.definitions,
      assets->ground_items.definition_count,
      assets->player_parameters.experience_threshold);
  else if (!game->world.player.parameters_initialized)
    success = sf_player_apply_initial_parameters(
      &game->world.player, &assets->player_parameters);
  if (success && saved_game)
    success = sf_player_restore_magic(
      &game->world.player, &saved_game->magic);
  if (success && saved_game)
    success = sf_player_restore_companions(
      &game->world.player, &saved_game->player,
      &saved_game->companions);
  if (success)
    (void) sf_player_magic_set_targeting(
      &game->world.player.magic, true);
  if (success) {
    sf_world_state_bind_transports(&game->world, &assets->transports);
    sf_world_state_bind_collision(
      &game->world, &assets->ground, &assets->objects);
    success = sf_world_state_bind_ground_items(
      &game->world, assets->ground_items.definitions,
      assets->ground_items.definition_count);
  }
  if (success && travel)
    sf_world_state_enter(
      &game->world, entry->world_x, entry->world_y,
      (uint8_t) entry->direction);
  if (success) success = travel
    ? sf_world_state_change_scenario(
        &game->world, scenario_id, entry_key,
        &assets->scenario, assets->script)
    : saved_game
      ? sf_world_bind_saved_scenario(
          &game->world, &assets->scenario,
          assets->script, saved_game)
      : sf_world_state_bind_scenario(
          &game->world, &assets->scenario, assets->script);
  if (success && !travel)
    sf_world_state_enter(
      &game->world, entry->world_x, entry->world_y,
      (uint8_t) entry->direction);
  if (success) success = sf_world_state_bind_companion(
    &game->world, &assets->companion_profile);
  if (success) success = sf_gameplay_screen_init(
    screen, assets, &game->world);
  if (success && travel)
    sf_scenario_travel_clear(&game->world.travel_request);
  return success;
}
