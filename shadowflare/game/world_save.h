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

#ifndef SHADOWFLARE_GAME_WORLD_SAVE_H
#define SHADOWFLARE_GAME_WORLD_SAVE_H

#include "data/save_game.h"
#include "game/world.h"

#include <stdbool.h>

bool sf_world_prepare_save_load(
  SfWorldState *world, const SfSavedGame *saved);
bool sf_world_bind_saved_scenario(
  SfWorldState *world,
  const SfMctScenario *scenario, const SfScsScript *script,
  const SfSavedGame *saved);

#endif
