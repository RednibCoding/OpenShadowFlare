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

#ifndef SHADOWFLARE_GAME_INPUT_H
#define SHADOWFLARE_GAME_INPUT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct SfGameInput {
  int32_t pointed_actor_id;
  int16_t pointer_x;
  int16_t pointer_y;
  bool pointer_active;
  bool world_pointer_resolved;
  bool pointer_primary_pressed;
  bool pointer_primary_down;
  bool up_pressed;
  bool down_pressed;
  bool left_pressed;
  bool right_pressed;
  bool confirm_pressed;
  bool cancel_pressed;
  bool backspace_pressed;
  bool delete_pressed;
  bool pace_toggle_pressed;
  char text[16];
  uint8_t text_length;
} SfGameInput;

#endif
