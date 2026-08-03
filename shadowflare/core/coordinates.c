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

int32_t sf_floor_divide(int32_t numerator, int32_t denominator) {
  int32_t result;
  if (denominator <= 0) return 0;
  result = numerator / denominator;
  if (numerator < 0 && numerator % denominator != 0) --result;
  return result;
}
