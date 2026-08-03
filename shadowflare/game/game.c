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

enum {
  SF_TITLE_ENTRY_COUNT = 3,
  SF_TITLE_EXIT_ENTRY = 2
};

void sf_game_init(SfGame *game) {
  if (!game) return;
  game->mode = SF_GAME_MODE_TITLE;
  game->ticks = 0u;
  game->title_selection = 0u;
  game->quit_requested = false;
}

void sf_game_update(SfGame *game, const SfGameInput *input) {
  if (!game || !input) return;
  ++game->ticks;
  if (input->up_pressed) {
    game->title_selection = game->title_selection == 0u
      ? SF_TITLE_ENTRY_COUNT - 1u
      : (uint8_t) (game->title_selection - 1u);
  }
  if (input->down_pressed) {
    game->title_selection =
      (uint8_t) ((game->title_selection + 1u) % SF_TITLE_ENTRY_COUNT);
  }
  if (input->cancel_pressed ||
      (input->confirm_pressed &&
       game->title_selection == SF_TITLE_EXIT_ENTRY)) {
    game->quit_requested = true;
  }
}
