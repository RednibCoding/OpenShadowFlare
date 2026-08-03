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

#ifndef SHADOWFLARE_SCREENS_GAMEPLAY_SCENE_H
#define SHADOWFLARE_SCREENS_GAMEPLAY_SCENE_H

#include "assets/gameplay_assets.h"
#include "game/world.h"
#include "render/renderer.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_GAMEPLAY_VISIBLE_OBJECT_LIMIT 256u
#define SF_GAMEPLAY_DRAW_ENTRY_LIMIT \
  (SF_GAMEPLAY_VISIBLE_OBJECT_LIMIT + SF_MCT_PERSON_LIMIT + 1u)

typedef struct SfGameplayScene {
  uint16_t visible_objects[SF_GAMEPLAY_DRAW_ENTRY_LIMIT];
  uint16_t shadow_objects[SF_GAMEPLAY_DRAW_ENTRY_LIMIT];
  uint8_t translucent_objects[SF_GAMEPLAY_DRAW_ENTRY_LIMIT];
  uint16_t visible_count;
  uint16_t shadow_count;
} SfGameplayScene;

bool sf_gameplay_scene_update(
  SfGameplayScene *scene, const SfGameplayAssets *assets,
  const SfWorldState *world, const SfWorldRenderView *view,
  uint16_t interpolation);
void sf_gameplay_scene_draw(
  const SfGameplayScene *scene, SfRenderer *renderer,
  const SfGameplayAssets *assets, const SfWorldState *world,
  const SfWorldRenderView *view, uint16_t interpolation,
  const SfRect *clip);

#endif
