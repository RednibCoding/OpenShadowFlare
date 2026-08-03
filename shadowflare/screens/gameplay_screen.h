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

#ifndef SHADOWFLARE_SCREENS_GAMEPLAY_SCREEN_H
#define SHADOWFLARE_SCREENS_GAMEPLAY_SCREEN_H

#include "assets/gameplay_assets.h"
#include "game/game.h"
#include "render/renderer.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_GAMEPLAY_VISIBLE_OBJECT_LIMIT 256u
#define SF_GAMEPLAY_DRAW_ENTRY_LIMIT (SF_GAMEPLAY_VISIBLE_OBJECT_LIMIT + 1u)

typedef struct SfGameplayScreen {
  uint16_t visible_objects[SF_GAMEPLAY_DRAW_ENTRY_LIMIT];
  uint16_t shadow_objects[SF_GAMEPLAY_DRAW_ENTRY_LIMIT];
  uint8_t translucent_objects[SF_GAMEPLAY_DRAW_ENTRY_LIMIT];
  uint16_t visible_count;
  uint16_t shadow_count;
  uint32_t rendered_animation_frame;
  int32_t rendered_player_x;
  int32_t rendered_player_y;
  int32_t rendered_camera_x;
  int32_t rendered_camera_y;
  uint8_t rendered_motion;
  uint8_t rendered_direction;
  SfRect player_damage;
  bool drawn;
} SfGameplayScreen;

bool sf_gameplay_screen_init(
  SfGameplayScreen *screen, const SfGameplayAssets *assets,
  const SfWorldState *world);
void sf_gameplay_screen_draw(
  SfGameplayScreen *screen, SfRenderer *renderer,
  const SfGameplayAssets *assets, const SfGame *game,
  uint16_t interpolation);

#endif
