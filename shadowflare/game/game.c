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

#include "game/game.h"

#include "game/character_create.h"
#include "game/load_game.h"
#include "game/title.h"

#include <string.h>

void sf_game_init(SfGame *game, const SfGameConfig *config) {
  if (!game) return;
  memset(game, 0, sizeof(*game));
  if (config) game->config = *config;
  game->load_game.selected_file_slot = -1;
  game->player_gender = 1u;
  game->mode = SF_GAME_MODE_TITLE;
  sf_title_state_init(game);
}

void sf_game_update(SfGame *game, const SfGameInput *input) {
  SfGameMode previous_mode;
  if (!game || !input) return;
  ++game->ticks;
  previous_mode = game->mode;
  if (game->mode == SF_GAME_MODE_TITLE) {
    sf_title_state_update(game, input);
    if (previous_mode != game->mode &&
        game->mode == SF_GAME_MODE_CHARACTER_SELECT)
      sf_character_create_state_init(game);
    if (previous_mode != game->mode &&
        game->mode == SF_GAME_MODE_LOAD_GAME)
      sf_load_game_state_init(game);
  } else if (game->mode == SF_GAME_MODE_CHARACTER_SELECT) {
    game->title.sound_events = 0u;
    sf_character_create_state_update(game, input);
  } else if (game->mode == SF_GAME_MODE_LOAD_GAME) {
    game->title.sound_events = 0u;
    game->character_create.sound_events = 0u;
    sf_load_game_state_update(game, input);
  } else if (game->mode == SF_GAME_MODE_LOADING) {
    game->title.sound_events = 0u;
    game->character_create.sound_events = 0u;
    game->load_game.sound_events = 0u;
    sf_world_state_init(&game->world, 0, 0, game->player_gender);
    if (game->load_game.selected_file_slot < 0)
      sf_player_set_identity(
        &game->world.player, game->character_create.name, 0x10);
    game->mode = SF_GAME_MODE_GAMEPLAY;
  } else {
    game->title.sound_events = 0u;
    game->character_create.sound_events = 0u;
    sf_world_state_update(&game->world, input);
  }
}

void sf_game_saved_catalog_changed(
    SfGame *game, const uint8_t *file_slots, const uint8_t *genders,
    uint8_t saved_game_count) {
  uint8_t index;
  if (!game) return;
  if (saved_game_count > 6u) saved_game_count = 6u;
  for (index = 0u; index < 6u; ++index)
    game->config.saved_game_file_slots[index] =
      file_slots && index < saved_game_count ? file_slots[index] : UINT8_MAX;
  for (index = 0u; index < 6u; ++index)
    game->config.saved_game_genders[index] =
      genders && index < saved_game_count && genders[index] == 1u ? 1u : 0u;
  game->config.saved_game_count = saved_game_count;
  game->config.next_save_available = saved_game_count < 6u;
  game->load_game.selected_file_slot = -1;
  if (saved_game_count == 0u) {
    game->load_game.selection = 0u;
  }
  else if (game->load_game.selection >= saved_game_count)
    game->load_game.selection = (uint8_t) (saved_game_count - 1u);
}

bool sf_game_recover_saved_game_load_failure(SfGame *game) {
  uint8_t selection;
  if (!game || game->mode != SF_GAME_MODE_GAMEPLAY ||
      game->load_game.selected_file_slot < 0) return false;
  selection = game->load_game.selected_save;
  game->mode = SF_GAME_MODE_LOAD_GAME;
  sf_load_game_state_init(game);
  if (selection < game->config.saved_game_count)
    game->load_game.selection = selection;
  return true;
}
