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

#ifndef SHADOWFLARE_SCREENS_GAMEPLAY_OBJECT_VISUAL_H
#define SHADOWFLARE_SCREENS_GAMEPLAY_OBJECT_VISUAL_H

#include "assets/gameplay_assets.h"
#include "game/world.h"
#include "render/renderer.h"

#include <stdbool.h>

typedef struct SfGameplayObjectVisual {
  const SfNjpDecodedResource *resource;
  const SfNjpDecodedPattern *pattern;
} SfGameplayObjectVisual;

bool sf_gameplay_object_visual_find(
  const SfGameplayAssets *assets, const SfMapObject *object,
  bool shadow, SfGameplayObjectVisual *visual);
bool sf_gameplay_object_visual_visible(
  const SfGameplayObjectVisual *visual, const SfMapObject *object,
  const SfWorldRenderView *view, bool shadow);
bool sf_gameplay_object_visual_intersects(
  const SfGameplayObjectVisual *visual, const SfMapObject *object,
  const SfWorldRenderView *view, SfRect rectangle);

#endif
