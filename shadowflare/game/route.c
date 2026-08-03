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

#include "game/route.h"

#include "game/movement.h"
#include "game/route_tables.h"

void sf_route_reset(SfRouteController *controller) {
  if (!controller) return;
  controller->movement = SF_CARDINAL_NONE;
  controller->wall = SF_CARDINAL_NONE;
}

SfRouteStep sf_route_advance_query(
    SfRouteController *controller, const SfCollisionQuery *query,
    SfObjectBounds bounds, SfWorldPoint position,
    SfWorldPoint destination, uint32_t speed) {
  SfWorldPoint attempted;
  SfCollisionSweep sweep;
  if (!controller) return (SfRouteStep) {position, false, false, false, false};
  if (position.x == destination.x && position.y == destination.y) {
    sf_route_reset(controller);
    return (SfRouteStep) {position, true, false, false, false};
  }
  if (speed == 0u) {
    sf_route_reset(controller);
    return (SfRouteStep) {position, false, false, false, false};
  }
  if (controller->movement == SF_CARDINAL_NONE) {
    attempted = sf_movement_step_toward(
      position, destination, speed).position;
    sweep = sf_collision_query_sweep(
      query, position, attempted, bounds, SF_CARDINAL_NONE);
    if (!sweep.collided) return (SfRouteStep) {
      sweep.position,
      sweep.position.x == destination.x && sweep.position.y == destination.y,
      sweep.position.x != position.x || sweep.position.y != position.y,
      true, false};
    {
      const SfRouteEdgeState edge = sf_route_initial_edge(
        query, bounds, position, attempted, sweep.position);
      if (edge.movement == SF_CARDINAL_NONE) {
        if (sweep.position.x != position.x || sweep.position.y != position.y)
          return (SfRouteStep) {
            sweep.position, false, true, true, true};
        sf_route_reset(controller);
        return (SfRouteStep) {position, false, false, false, true};
      }
      controller->movement = edge.movement;
      controller->wall = edge.wall;
      return (SfRouteStep) {
        sweep.position, false,
        sweep.position.x != position.x || sweep.position.y != position.y,
        true, true};
    }
  }
  if (sf_route_should_leave_edge(
        position, destination, controller->movement, controller->wall)) {
    sf_route_reset(controller);
    return (SfRouteStep) {position, false, false, false, false};
  }
  {
    const SfWorldPoint vector = sf_cardinal_vector(controller->movement);
    const SfWorldPoint edge_target = {
      position.x + vector.x * (int32_t) speed,
      position.y + vector.y * (int32_t) speed};
    sweep = sf_collision_query_sweep(
      query, position, edge_target, bounds, controller->wall);
    if (sweep.position.x == position.x && sweep.position.y == position.y) {
      const SfCardinalDirection previous_movement = controller->movement;
      controller->movement = sf_cardinal_opposite(controller->wall);
      controller->wall = previous_movement;
      return (SfRouteStep) {position, false, false, true, sweep.collided};
    }
    if ((vector.x == 0 && sweep.position.x != position.x) ||
        (vector.y == 0 && sweep.position.y != position.y)) {
      const SfRouteEdgeState next = sf_route_after_side_step(
        sweep.position, destination, controller->wall);
      controller->movement = next.movement;
      controller->wall = next.wall;
    }
    return (SfRouteStep) {
      sweep.position,
      sweep.position.x == destination.x && sweep.position.y == destination.y,
      true, true, sweep.collided};
  }
}

SfRouteStep sf_route_advance(
    SfRouteController *controller, const SfCollisionWorld *world,
    SfObjectBounds bounds, SfWorldPoint position,
    SfWorldPoint destination, uint32_t speed) {
  const SfCollisionQuery query = {world, NULL, INT32_MIN, 0u};
  return sf_route_advance_query(
    controller, &query, bounds, position, destination, speed);
}
