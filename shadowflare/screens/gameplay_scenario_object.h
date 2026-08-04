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

#ifndef SHADOWFLARE_SCREENS_GAMEPLAY_SCENARIO_OBJECT_H
#define SHADOWFLARE_SCREENS_GAMEPLAY_SCENARIO_OBJECT_H

#include "assets/scenario_object_assets.h"
#include "game/scenario_object.h"
#include "game/world.h"
#include "render/renderer.h"

#include <stdbool.h>

bool sf_gameplay_scenario_object_visible(
  const SfScenarioObjectAssets *assets, const SfScenarioObject *object,
  const SfWorldRenderView *view, bool shadow);
void sf_gameplay_scenario_object_draw(
  SfRenderer *renderer, const SfScenarioObjectAssets *assets,
  const SfScenarioObject *object, const SfWorldRenderView *view,
  bool shadow, bool hovered, const SfRect *clip);
bool sf_gameplay_scenario_object_pixel_hit(
  const SfScenarioObjectAssets *assets, const SfScenarioObject *object,
  const SfWorldRenderView *view, int pointer_x, int pointer_y,
  int half_size, bool *exact);

#endif
