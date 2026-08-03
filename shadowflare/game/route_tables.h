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

#ifndef SHADOWFLARE_GAME_ROUTE_TABLES_H
#define SHADOWFLARE_GAME_ROUTE_TABLES_H

#include "game/collision.h"

typedef struct SfRouteEdgeState {
  SfCardinalDirection movement;
  SfCardinalDirection wall;
} SfRouteEdgeState;

SfRouteEdgeState sf_route_initial_edge(
  const SfCollisionWorld *world, SfObjectBounds bounds,
  SfWorldPoint start, SfWorldPoint attempted, SfWorldPoint contact);
bool sf_route_should_leave_edge(
  SfWorldPoint position, SfWorldPoint destination,
  SfCardinalDirection movement, SfCardinalDirection wall);
SfRouteEdgeState sf_route_after_side_step(
  SfWorldPoint position, SfWorldPoint destination,
  SfCardinalDirection wall);

#endif
