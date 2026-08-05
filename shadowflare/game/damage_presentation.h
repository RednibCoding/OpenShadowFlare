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

#ifndef SHADOWFLARE_GAME_DAMAGE_PRESENTATION_H
#define SHADOWFLARE_GAME_DAMAGE_PRESENTATION_H

#include <stdbool.h>
#include <stdint.h>

typedef struct SfDamagePresentationState {
  int32_t action;
  int32_t counter;
  int32_t reaction_duration;
  int32_t reaction_additive;
  int32_t event_number;
  uint8_t reaction_stage;
  bool action_locked;
  bool reaction_motion;
} SfDamagePresentationState;

#endif
