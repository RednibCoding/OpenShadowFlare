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

#ifndef SHADOWFLARE_GAME_MOVEMENT_H
#define SHADOWFLARE_GAME_MOVEMENT_H

#include "core/coordinates.h"
#include "core/bounds.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct SfMovementStep {
  SfWorldPoint position;
  bool moved;
  bool arrived;
} SfMovementStep;

SfMovementStep sf_movement_step_toward(
  SfWorldPoint position, SfWorldPoint destination, uint32_t speed);
uint8_t sf_movement_direction(SfWorldPoint from, SfWorldPoint to);
int32_t sf_movement_point_distance(SfWorldPoint first, SfWorldPoint second);
bool sf_movement_point_at_distance(
  SfWorldPoint from, SfWorldPoint toward, uint32_t distance,
  SfWorldPoint *result);
bool sf_movement_vector_at_distance(
  SfWorldPoint direction, uint32_t distance, SfWorldPoint *result);
int32_t sf_movement_bounds_distance(
  SfWorldPoint first_position, SfObjectBounds first_bounds,
  SfWorldPoint second_position, SfObjectBounds second_bounds);

#endif
