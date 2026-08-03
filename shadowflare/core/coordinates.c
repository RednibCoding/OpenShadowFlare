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

#include "core/coordinates.h"

SfScreenPoint sf_world_to_screen(SfWorldPoint point) {
  SfScreenPoint result;
  result.x = (int32_t) (((int64_t) point.x - point.y) * 15 / 100);
  result.y = (int32_t) (((int64_t) point.x + point.y) * 10 / 100);
  return result;
}

SfWorldPoint sf_screen_to_world(SfScreenPoint point) {
  SfWorldPoint result;
  result.x = (point.x * 10 + point.y * 15) / 3;
  result.y = (point.y * 15 - point.x * 10) / 3;
  return result;
}

static int32_t sf_world_axis_interpolate(
    int32_t previous, int32_t current, uint16_t interpolation) {
  int64_t distance;
  if (interpolation > 1000u) interpolation = 1000u;
  distance = ((int64_t) current - previous) * interpolation;
  distance += distance >= 0 ? 500 : -500;
  return previous + (int32_t) (distance / 1000);
}

SfWorldPoint sf_world_point_interpolate(
    SfWorldPoint previous, SfWorldPoint current, uint16_t interpolation) {
  SfWorldPoint result;
  result.x = sf_world_axis_interpolate(
    previous.x, current.x, interpolation);
  result.y = sf_world_axis_interpolate(
    previous.y, current.y, interpolation);
  return result;
}

int32_t sf_floor_divide(int32_t numerator, int32_t denominator) {
  int32_t result;
  if (denominator <= 0) return 0;
  result = numerator / denominator;
  if (numerator < 0 && numerator % denominator != 0) --result;
  return result;
}
