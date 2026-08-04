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

#ifndef SHADOWFLARE_SCREENS_GAMEPLAY_PLAYER_H
#define SHADOWFLARE_SCREENS_GAMEPLAY_PLAYER_H

#include "assets/player_assets.h"
#include "game/world.h"
#include "render/renderer.h"

#include <stdbool.h>

SfRect sf_gameplay_player_bounds(
  const SfPlayerAssets *assets, const SfWorldState *world,
  const SfWorldRenderView *view);
void sf_gameplay_player_draw(
  SfRenderer *renderer, const SfPlayerAssets *assets,
  const SfWorldState *world, const SfWorldRenderView *view,
  bool shadow, const SfRect *clip);

#endif
