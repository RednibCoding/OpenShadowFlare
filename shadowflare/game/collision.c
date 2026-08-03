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

#include "game/collision.h"

#include <limits.h>

static int32_t sf_collision_clamp(
    int32_t value, int32_t minimum, int32_t maximum) {
  if (value < minimum) return minimum;
  if (value > maximum) return maximum;
  return value;
}

static bool sf_collision_overlaps(
    int32_t first_left, int32_t first_top,
    int32_t first_right, int32_t first_bottom,
    int32_t second_left, int32_t second_top,
    int32_t second_right, int32_t second_bottom) {
  return first_left <= second_right && second_left <= first_right &&
    first_top <= second_bottom && second_top <= first_bottom;
}

SfWorldPoint sf_cardinal_vector(SfCardinalDirection direction) {
  switch (direction) {
    case SF_CARDINAL_NORTH: return (SfWorldPoint) {0, -1};
    case SF_CARDINAL_SOUTH: return (SfWorldPoint) {0, 1};
    case SF_CARDINAL_WEST: return (SfWorldPoint) {-1, 0};
    case SF_CARDINAL_EAST: return (SfWorldPoint) {1, 0};
    default: return (SfWorldPoint) {0, 0};
  }
}

SfCardinalDirection sf_cardinal_opposite(SfCardinalDirection direction) {
  switch (direction) {
    case SF_CARDINAL_NORTH: return SF_CARDINAL_SOUTH;
    case SF_CARDINAL_SOUTH: return SF_CARDINAL_NORTH;
    case SF_CARDINAL_WEST: return SF_CARDINAL_EAST;
    case SF_CARDINAL_EAST: return SF_CARDINAL_WEST;
    default: return SF_CARDINAL_NONE;
  }
}

static bool sf_collision_ground_walkable(
    const SfGroundMap *ground,
    int32_t left, int32_t top, int32_t right, int32_t bottom,
    bool exclude_special_objects) {
  int32_t first_x;
  int32_t first_y;
  int32_t last_x;
  int32_t last_y;
  int32_t y;
  if (!ground || ground->judge_width <= 0 || ground->judge_height <= 0)
    return true;
  if (ground->base_magnification_x <= 0 ||
      ground->base_magnification_y <= 0) return false;
  first_x = left / ground->base_magnification_x - 1;
  first_y = top / ground->base_magnification_y - 1;
  last_x = right / ground->base_magnification_x + 1;
  last_y = bottom / ground->base_magnification_y + 1;
  first_x = sf_collision_clamp(
    first_x, ground->judge_offset_x,
    ground->judge_offset_x + ground->judge_width - 1);
  last_x = sf_collision_clamp(
    last_x, ground->judge_offset_x,
    ground->judge_offset_x + ground->judge_width - 1);
  first_y = sf_collision_clamp(
    first_y, ground->judge_offset_y,
    ground->judge_offset_y + ground->judge_height - 1);
  last_y = sf_collision_clamp(
    last_y, ground->judge_offset_y,
    ground->judge_offset_y + ground->judge_height - 1);
  for (y = first_y; y <= last_y; ++y) {
    int32_t x;
    for (x = first_x; x <= last_x; ++x) {
      const uint8_t flags = sf_ground_movement_flags(ground, x, y);
      if ((flags & 1u) == 0u ||
          (exclude_special_objects && (flags & 2u) != 0u)) continue;
      if (sf_collision_overlaps(
            left, top, right, bottom,
            x * ground->base_magnification_x,
            y * ground->base_magnification_y,
            (x + 1) * ground->base_magnification_x - 1,
            (y + 1) * ground->base_magnification_y - 1)) return false;
    }
  }
  return true;
}

bool sf_collision_position_walkable(
    const SfCollisionWorld *world, SfWorldPoint position,
    SfObjectBounds bounds, bool exclude_special_objects) {
  const int32_t left = position.x + bounds.left;
  const int32_t top = position.y + bounds.top;
  const int32_t right = position.x + bounds.right;
  const int32_t bottom = position.y + bounds.bottom;
  uint16_t object;
  if (!world) return true;
  if (world->objects) {
    for (object = 0u; object < world->objects->count; ++object) {
      const SfMapObject *item = &world->objects->objects[object];
      if ((item->status & 1) == 0 ||
          (exclude_special_objects && (item->status & 3) == 3)) continue;
      if (sf_collision_overlaps(
            left, top, right, bottom,
            item->world_x + item->judgement.left,
            item->world_y + item->judgement.top,
            item->world_x + item->judgement.right,
            item->world_y + item->judgement.bottom)) return false;
    }
  }
  return sf_collision_ground_walkable(
    world->ground, left, top, right, bottom, exclude_special_objects);
}

bool sf_collision_can_step(
    const SfCollisionWorld *world, SfWorldPoint position,
    SfObjectBounds bounds, SfCardinalDirection direction) {
  const SfWorldPoint vector = sf_cardinal_vector(direction);
  position.x += vector.x;
  position.y += vector.y;
  return sf_collision_position_walkable(world, position, bounds, false);
}

static SfWorldPoint sf_collision_segment_point(
    SfWorldPoint start, SfWorldPoint end,
    bool horizontal, int32_t step, int32_t count) {
  SfWorldPoint result = start;
  const int32_t dx = end.x - start.x;
  const int32_t dy = end.y - start.y;
  if (step == count) return end;
  if (horizontal) {
    result.x += dx < 0 ? -step : step;
    result.y += (result.x - start.x) * dy / dx;
    result.y = sf_collision_clamp(
      result.y, start.y < end.y ? start.y : end.y,
      start.y > end.y ? start.y : end.y);
  } else {
    result.y += dy < 0 ? -step : step;
    result.x += (result.y - start.y) * dx / dy;
    result.x = sf_collision_clamp(
      result.x, start.x < end.x ? start.x : end.x,
      start.x > end.x ? start.x : end.x);
  }
  return result;
}

SfCollisionSweep sf_collision_sweep(
    const SfCollisionWorld *world, SfWorldPoint start, SfWorldPoint end,
    SfObjectBounds bounds, SfCardinalDirection wall_direction) {
  const int32_t dx = end.x - start.x;
  const int32_t dy = end.y - start.y;
  const int32_t ax = dx < 0 ? -dx : dx;
  const int32_t ay = dy < 0 ? -dy : dy;
  const bool horizontal = ay < ax;
  const int32_t count = horizontal ? ax : ay;
  SfWorldPoint contact = start;
  int32_t contact_step = 0;
  int32_t step;
  bool collided = false;
  bool valid_wall;
  SfWorldPoint wall;
  bool side_blocked = false;
  if (count == 0) return (SfCollisionSweep) {start, false};
  for (step = 1; step <= count; ++step) {
    const SfWorldPoint point = sf_collision_segment_point(
      start, end, horizontal, step, count);
    if (!sf_collision_position_walkable(world, point, bounds, false)) {
      collided = true;
      break;
    }
    contact = point;
    contact_step = step;
  }
  valid_wall = (start.y == end.y && start.x != end.x &&
      (wall_direction == SF_CARDINAL_NORTH ||
       wall_direction == SF_CARDINAL_SOUTH)) ||
    (start.x == end.x && start.y != end.y &&
      (wall_direction == SF_CARDINAL_WEST ||
       wall_direction == SF_CARDINAL_EAST));
  if (!valid_wall) return (SfCollisionSweep) {contact, collided};
  wall = sf_cardinal_vector(wall_direction);
  for (step = 0; step <= count; ++step) {
    SfWorldPoint side = sf_collision_segment_point(
      start, end, horizontal, step, count);
    side.x += wall.x;
    side.y += wall.y;
    if (!sf_collision_position_walkable(world, side, bounds, false)) {
      side_blocked = true;
      break;
    }
  }
  if (side_blocked) {
    for (step = 0; step <= contact_step; ++step) {
      SfWorldPoint side = sf_collision_segment_point(
        start, end, horizontal, step, count);
      side.x += wall.x;
      side.y += wall.y;
      if (sf_collision_position_walkable(world, side, bounds, false)) {
        contact = side;
        break;
      }
    }
    if (step > contact_step)
      return (SfCollisionSweep) {contact, collided};
  } else {
    contact.x += wall.x;
    contact.y += wall.y;
  }
  if (!sf_collision_position_walkable(world, contact, bounds, false)) {
    contact.x -= wall.x;
    contact.y -= wall.y;
  }
  return (SfCollisionSweep) {
    contact, collided || contact.x != end.x || contact.y != end.y};
}
