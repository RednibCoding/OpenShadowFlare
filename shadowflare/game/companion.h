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

#ifndef SHADOWFLARE_GAME_COMPANION_H
#define SHADOWFLARE_GAME_COMPANION_H

#include "core/coordinates.h"
#include "data/companion_parameters.h"
#include "game/collision.h"
#include "game/player.h"
#include "game/route.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_COMPANION_CHARACTER_NUMBER 16000000

typedef enum SfCompanionMotion {
  SF_COMPANION_IDLE = 0,
  SF_COMPANION_WALKING = 1,
  SF_COMPANION_RUNNING = 2
} SfCompanionMotion;

typedef struct SfCompanionState {
  SfCompanionProfile profile;
  SfWorldPoint position;
  SfWorldPoint previous_position;
  SfObjectBounds judgement;
  SfRouteController route;
  int32_t current_life;
  uint32_t action_counter;
  uint32_t animation_frame;
  uint8_t direction;
  SfCompanionMotion motion;
  uint8_t close_linger_updates;
  bool inactive;
  bool valid;
} SfCompanionState;

bool sf_companion_init(
  SfCompanionState *companion, const SfCompanionProfile *profile,
  SfWorldPoint position, uint8_t direction, bool defeated);
bool sf_companion_bind_profile(
  SfCompanionState *companion, const SfCompanionProfile *profile,
  SfWorldPoint position, uint8_t direction, bool defeated);
void sf_companion_relocate(
  SfCompanionState *companion, SfWorldPoint position, uint8_t direction);
void sf_companion_toggle_activity(SfCompanionState *companion);
void sf_companion_update_follow(
  SfCompanionState *companion, const SfPlayerState *owner,
  const SfCollisionQuery *collision);
SfWorldPoint sf_companion_render_position(
  const SfCompanionState *companion, uint16_t interpolation);

#endif
