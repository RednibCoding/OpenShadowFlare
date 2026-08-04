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

#ifndef SHADOWFLARE_RUNTIME_GAMEPLAY_RUNTIME_H
#define SHADOWFLARE_RUNTIME_GAMEPLAY_RUNTIME_H

#include "assets/gameplay_assets.h"
#include "core/arena.h"
#include "data/save_game.h"
#include "game/game.h"
#include "screens/gameplay_screen.h"

#include <stdbool.h>

bool sf_gameplay_runtime_load(
  SfGameplayAssets *assets, SfGameplayScreen *screen,
  SfArena *arena, const char *data_root, SfGame *game,
  const SfSavedGame *saved_game,
  const SfScenarioTravelRequest *travel);

#endif
