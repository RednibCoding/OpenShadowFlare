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

#ifndef SHADOWFLARE_GAME_COLLISION_H
#define SHADOWFLARE_GAME_COLLISION_H

#include "core/bounds.h"
#include "core/coordinates.h"
#include "data/gnd.h"
#include "data/obl.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum SfCardinalDirection {
  SF_CARDINAL_NONE = 0,
  SF_CARDINAL_NORTH = 1,
  SF_CARDINAL_SOUTH = 2,
  SF_CARDINAL_WEST = 3,
  SF_CARDINAL_EAST = 4
} SfCardinalDirection;

typedef struct SfCollisionWorld {
  const SfGroundMap *ground;
  const SfObjectMap *objects;
} SfCollisionWorld;

typedef struct SfCollisionSweep {
  SfWorldPoint position;
  bool collided;
} SfCollisionSweep;

SfWorldPoint sf_cardinal_vector(SfCardinalDirection direction);
SfCardinalDirection sf_cardinal_opposite(SfCardinalDirection direction);
bool sf_collision_position_walkable(
  const SfCollisionWorld *world, SfWorldPoint position,
  SfObjectBounds bounds, bool exclude_special_objects);
bool sf_collision_can_step(
  const SfCollisionWorld *world, SfWorldPoint position,
  SfObjectBounds bounds, SfCardinalDirection direction);
SfCollisionSweep sf_collision_sweep(
  const SfCollisionWorld *world, SfWorldPoint start, SfWorldPoint end,
  SfObjectBounds bounds, SfCardinalDirection wall_direction);

#endif
