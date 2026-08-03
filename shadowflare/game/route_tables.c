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

#include "game/route_tables.h"

#define SF_ROUTE_EDGE(move, side) \
  ((SfRouteEdgeState) {(move), (side)})

static SfRouteEdgeState sf_route_stopped_edge(
    const SfCollisionWorld *world, SfObjectBounds bounds,
    SfWorldPoint start, int x_sign, int y_sign) {
  if (x_sign > 0 && y_sign > 0) {
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_EAST))
      return SF_ROUTE_EDGE(SF_CARDINAL_EAST, SF_CARDINAL_SOUTH);
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_SOUTH))
      return SF_ROUTE_EDGE(SF_CARDINAL_SOUTH, SF_CARDINAL_EAST);
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_WEST))
      return SF_ROUTE_EDGE(SF_CARDINAL_WEST, SF_CARDINAL_SOUTH);
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_NORTH))
      return SF_ROUTE_EDGE(SF_CARDINAL_NORTH, SF_CARDINAL_EAST);
  } else if (x_sign > 0 && y_sign < 0) {
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_EAST))
      return SF_ROUTE_EDGE(SF_CARDINAL_EAST, SF_CARDINAL_NORTH);
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_NORTH))
      return SF_ROUTE_EDGE(SF_CARDINAL_NORTH, SF_CARDINAL_EAST);
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_WEST))
      return SF_ROUTE_EDGE(SF_CARDINAL_WEST, SF_CARDINAL_NORTH);
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_SOUTH))
      return SF_ROUTE_EDGE(SF_CARDINAL_SOUTH, SF_CARDINAL_EAST);
  } else if (x_sign < 0 && y_sign > 0) {
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_WEST))
      return SF_ROUTE_EDGE(SF_CARDINAL_WEST, SF_CARDINAL_SOUTH);
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_SOUTH))
      return SF_ROUTE_EDGE(SF_CARDINAL_SOUTH, SF_CARDINAL_WEST);
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_EAST))
      return SF_ROUTE_EDGE(SF_CARDINAL_EAST, SF_CARDINAL_SOUTH);
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_NORTH))
      return SF_ROUTE_EDGE(SF_CARDINAL_NORTH, SF_CARDINAL_WEST);
  } else if (x_sign < 0 && y_sign < 0) {
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_WEST))
      return SF_ROUTE_EDGE(SF_CARDINAL_WEST, SF_CARDINAL_NORTH);
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_NORTH))
      return SF_ROUTE_EDGE(SF_CARDINAL_NORTH, SF_CARDINAL_WEST);
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_EAST))
      return SF_ROUTE_EDGE(SF_CARDINAL_EAST, SF_CARDINAL_NORTH);
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_SOUTH))
      return SF_ROUTE_EDGE(SF_CARDINAL_SOUTH, SF_CARDINAL_WEST);
  }
  return SF_ROUTE_EDGE(SF_CARDINAL_NONE, SF_CARDINAL_NONE);
}

SfRouteEdgeState sf_route_initial_edge(
    const SfCollisionWorld *world, SfObjectBounds bounds,
    SfWorldPoint start, SfWorldPoint attempted, SfWorldPoint contact) {
  const int x_sign = attempted.x > start.x ? 1 :
    attempted.x < start.x ? -1 : 0;
  const int y_sign = attempted.y > start.y ? 1 :
    attempted.y < start.y ? -1 : 0;
  const bool stopped = contact.x == start.x && contact.y == start.y;
  if (stopped && x_sign != 0 && y_sign != 0)
    return sf_route_stopped_edge(world, bounds, start, x_sign, y_sign);
  if (x_sign > 0 && y_sign > 0) {
    if (contact.x == start.x && contact.y != start.y)
      return SF_ROUTE_EDGE(SF_CARDINAL_SOUTH, SF_CARDINAL_EAST);
    if (contact.y == start.y && contact.x != start.x)
      return SF_ROUTE_EDGE(SF_CARDINAL_EAST, SF_CARDINAL_SOUTH);
  } else if (x_sign > 0 && y_sign < 0) {
    if (contact.x == start.x && contact.y != start.y)
      return SF_ROUTE_EDGE(SF_CARDINAL_NORTH, SF_CARDINAL_EAST);
    if (contact.y == start.y && contact.x != start.x)
      return SF_ROUTE_EDGE(SF_CARDINAL_EAST, SF_CARDINAL_NORTH);
  } else if (x_sign < 0 && y_sign > 0) {
    if (contact.x == start.x && contact.y != start.y)
      return SF_ROUTE_EDGE(SF_CARDINAL_SOUTH, SF_CARDINAL_WEST);
    if (contact.y == start.y && contact.x != start.x)
      return SF_ROUTE_EDGE(SF_CARDINAL_WEST, SF_CARDINAL_SOUTH);
  } else if (x_sign < 0 && y_sign < 0) {
    if (contact.x == start.x && contact.y != start.y)
      return SF_ROUTE_EDGE(SF_CARDINAL_NORTH, SF_CARDINAL_WEST);
    if (contact.y == start.y && contact.x != start.x)
      return SF_ROUTE_EDGE(SF_CARDINAL_WEST, SF_CARDINAL_NORTH);
  } else if (x_sign == 0 && y_sign > 0 && contact.y == start.y) {
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_EAST))
      return SF_ROUTE_EDGE(SF_CARDINAL_EAST, SF_CARDINAL_SOUTH);
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_WEST))
      return SF_ROUTE_EDGE(SF_CARDINAL_WEST, SF_CARDINAL_SOUTH);
  } else if (x_sign == 0 && y_sign < 0 && contact.y == start.y) {
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_EAST))
      return SF_ROUTE_EDGE(SF_CARDINAL_EAST, SF_CARDINAL_NORTH);
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_WEST))
      return SF_ROUTE_EDGE(SF_CARDINAL_WEST, SF_CARDINAL_NORTH);
  } else if (y_sign == 0 && x_sign > 0 && contact.x == start.x) {
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_SOUTH))
      return SF_ROUTE_EDGE(SF_CARDINAL_SOUTH, SF_CARDINAL_EAST);
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_NORTH))
      return SF_ROUTE_EDGE(SF_CARDINAL_NORTH, SF_CARDINAL_EAST);
  } else if (y_sign == 0 && x_sign < 0 && contact.x == start.x) {
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_SOUTH))
      return SF_ROUTE_EDGE(SF_CARDINAL_SOUTH, SF_CARDINAL_WEST);
    if (sf_collision_can_step(
          world, start, bounds, SF_CARDINAL_NORTH))
      return SF_ROUTE_EDGE(SF_CARDINAL_NORTH, SF_CARDINAL_WEST);
  }
  return SF_ROUTE_EDGE(SF_CARDINAL_NONE, SF_CARDINAL_NONE);
}

bool sf_route_should_leave_edge(
    SfWorldPoint position, SfWorldPoint destination,
    SfCardinalDirection movement, SfCardinalDirection wall) {
  const int32_t dx = destination.x - position.x;
  const int32_t dy = destination.y - position.y;
  if (dy < 0) {
    if (dx < 0) return
      (movement == SF_CARDINAL_SOUTH && wall == SF_CARDINAL_EAST) ||
      (movement == SF_CARDINAL_EAST && wall == SF_CARDINAL_SOUTH);
    if (dx > 0) return
      (movement == SF_CARDINAL_SOUTH && wall == SF_CARDINAL_WEST) ||
      (movement == SF_CARDINAL_WEST && wall == SF_CARDINAL_SOUTH);
    return movement == SF_CARDINAL_SOUTH;
  }
  if (dy > 0) {
    if (dx < 0) return
      (movement == SF_CARDINAL_NORTH && wall == SF_CARDINAL_EAST) ||
      (movement == SF_CARDINAL_EAST && wall == SF_CARDINAL_NORTH);
    if (dx > 0) return
      (movement == SF_CARDINAL_NORTH && wall == SF_CARDINAL_WEST) ||
      (movement == SF_CARDINAL_WEST && wall == SF_CARDINAL_NORTH);
    return movement == SF_CARDINAL_NORTH;
  }
  if (dx < 0) return movement == SF_CARDINAL_EAST;
  if (dx > 0) return movement == SF_CARDINAL_WEST;
  return true;
}

SfRouteEdgeState sf_route_after_side_step(
    SfWorldPoint position, SfWorldPoint destination,
    SfCardinalDirection wall) {
  const int32_t dx = destination.x - position.x;
  const int32_t dy = destination.y - position.y;
  if (dy < 0 && dx < 0) {
    if (wall == SF_CARDINAL_EAST)
      return SF_ROUTE_EDGE(SF_CARDINAL_EAST, SF_CARDINAL_NORTH);
    if (wall == SF_CARDINAL_SOUTH)
      return SF_ROUTE_EDGE(SF_CARDINAL_SOUTH, SF_CARDINAL_WEST);
  } else if (dy < 0 && dx > 0) {
    if (wall == SF_CARDINAL_WEST)
      return SF_ROUTE_EDGE(SF_CARDINAL_WEST, SF_CARDINAL_NORTH);
    if (wall == SF_CARDINAL_SOUTH)
      return SF_ROUTE_EDGE(SF_CARDINAL_SOUTH, SF_CARDINAL_EAST);
  } else if (dy > 0 && dx < 0) {
    if (wall == SF_CARDINAL_EAST)
      return SF_ROUTE_EDGE(SF_CARDINAL_EAST, SF_CARDINAL_SOUTH);
    if (wall == SF_CARDINAL_NORTH)
      return SF_ROUTE_EDGE(SF_CARDINAL_NORTH, SF_CARDINAL_WEST);
  } else if (dy > 0 && dx > 0) {
    if (wall == SF_CARDINAL_WEST)
      return SF_ROUTE_EDGE(SF_CARDINAL_WEST, SF_CARDINAL_SOUTH);
    if (wall == SF_CARDINAL_NORTH)
      return SF_ROUTE_EDGE(SF_CARDINAL_NORTH, SF_CARDINAL_EAST);
  }
  return SF_ROUTE_EDGE(SF_CARDINAL_NONE, SF_CARDINAL_NONE);
}
