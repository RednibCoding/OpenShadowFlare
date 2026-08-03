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
#include "game/title.h"

#include <string.h>

void sf_game_init(SfGame *game, const SfGameConfig *config) {
  if (!game) return;
  memset(game, 0, sizeof(*game));
  if (config) game->config = *config;
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
  } else if (game->mode == SF_GAME_MODE_CHARACTER_SELECT) {
    game->title.sound_events = 0u;
    sf_character_create_state_update(game, input);
  } else {
    game->title.sound_events = 0u;
    game->character_create.sound_events = 0u;
  }
}
