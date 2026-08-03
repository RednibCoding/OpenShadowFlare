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

#ifndef SHADOWFLARE_GAME_ROUTE_H
#define SHADOWFLARE_GAME_ROUTE_H

#include "game/collision.h"

typedef struct SfRouteController {
  SfCardinalDirection movement;
  SfCardinalDirection wall;
} SfRouteController;

typedef struct SfRouteStep {
  SfWorldPoint position;
  bool reached_destination;
  bool moved;
  bool controller_active;
  bool collided;
} SfRouteStep;

void sf_route_reset(SfRouteController *controller);
SfRouteStep sf_route_advance(
  SfRouteController *controller, const SfCollisionWorld *world,
  SfObjectBounds bounds, SfWorldPoint position,
  SfWorldPoint destination, uint32_t speed);

#endif
