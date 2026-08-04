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

#ifndef SHADOWFLARE_SCREENS_GAMEPLAY_ENEMY_H
#define SHADOWFLARE_SCREENS_GAMEPLAY_ENEMY_H

#include "assets/scenario_enemy_assets.h"
#include "game/scenario_enemy.h"
#include "game/world.h"
#include "render/renderer.h"

#include <stdbool.h>

bool sf_gameplay_enemy_visible(
  const SfScenarioEnemyAssets *assets, const SfScenarioEnemy *enemy,
  const SfWorldRenderView *view, uint16_t interpolation, bool shadow);
void sf_gameplay_enemy_draw(
  SfRenderer *renderer, const SfScenarioEnemyAssets *assets,
  const SfScenarioEnemy *enemy, const SfWorldRenderView *view,
  uint16_t interpolation, bool shadow, const SfRect *clip);

#endif
