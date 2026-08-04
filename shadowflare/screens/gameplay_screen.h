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
#include "screens/gameplay_scene.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct SfGameplayScreen {
  SfGameplayScene scene;
  uint32_t rendered_animation_frame;
  uint32_t rendered_actor_frames[SF_MCT_PERSON_LIMIT];
  int32_t rendered_actor_x[SF_MCT_PERSON_LIMIT];
  int32_t rendered_actor_y[SF_MCT_PERSON_LIMIT];
  uint8_t rendered_actor_chart[SF_MCT_PERSON_LIMIT];
  bool rendered_actor_visible[SF_MCT_PERSON_LIMIT];
  int32_t rendered_player_x;
  int32_t rendered_player_y;
  int32_t rendered_camera_x;
  int32_t rendered_camera_y;
  int32_t rendered_hovered_actor_id;
  int32_t rendered_message_id;
  int32_t rendered_selected_option;
  int16_t rendered_pointer_x;
  int16_t rendered_pointer_y;
  uint8_t rendered_motion;
  uint8_t rendered_direction;
  bool rendered_pointer_active;
  bool rendered_message_active;
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
