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

#include "game/movement.h"

static uint32_t sf_integer_sqrt(uint64_t value) {
  uint64_t bit = UINT64_C(1) << 62u;
  uint64_t result = 0u;
  while (bit > value) bit >>= 2u;
  while (bit != 0u) {
    if (value >= result + bit) {
      value -= result + bit;
      result = (result >> 1u) + bit;
    } else {
      result >>= 1u;
    }
    bit >>= 2u;
  }
  return result > UINT32_MAX ? UINT32_MAX : (uint32_t) result;
}

SfMovementStep sf_movement_step_toward(
    SfWorldPoint position, SfWorldPoint destination, uint32_t speed) {
  const int64_t dx = (int64_t) destination.x - position.x;
  const int64_t dy = (int64_t) destination.y - position.y;
  const uint32_t distance = sf_integer_sqrt(
    (uint64_t) (dx * dx) + (uint64_t) (dy * dy));
  SfMovementStep result = {position, false, false};
  if (distance == 0u) {
    result.arrived = true;
    return result;
  }
  if (distance <= speed) {
    result.position = destination;
    result.moved = true;
    result.arrived = true;
    return result;
  }
  result.position.x += (int32_t) (dx * speed / distance);
  result.position.y += (int32_t) (dy * speed / distance);
  if (result.position.x == position.x && dx != 0)
    result.position.x += dx < 0 ? -1 : 1;
  if (result.position.y == position.y && dy != 0)
    result.position.y += dy < 0 ? -1 : 1;
  result.moved = result.position.x != position.x ||
    result.position.y != position.y;
  return result;
}

uint8_t sf_movement_direction(SfWorldPoint from, SfWorldPoint to) {
  const int64_t dx = (int64_t) to.x - from.x;
  const int64_t dy = (int64_t) to.y - from.y;
  const uint64_t ax = (uint64_t) (dx < 0 ? -dx : dx);
  const uint64_t ay = (uint64_t) (dy < 0 ? -dy : dy);
  if (ax == 0u && ay == 0u) return 1u;
  if (ay * 1000u <= ax * 414u) return dx >= 0 ? 1u : 5u;
  if (ay * 1000u >= ax * 2414u) return dy < 0 ? 3u : 7u;
  if (dx >= 0) return dy < 0 ? 2u : 0u;
  return dy < 0 ? 4u : 6u;
}
