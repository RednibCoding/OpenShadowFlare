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

#include "game/companion.h"

#include "game/movement.h"

#include <string.h>

#define SF_COMPANION_CLOSE_DISTANCE 160
#define SF_COMPANION_RUN_DISTANCE 600
#define SF_COMPANION_RELOCATE_DISTANCE 4000
#define SF_COMPANION_RELOCATE_OFFSET 200
#define SF_COMPANION_CLOSE_LINGER 5u

static void sf_companion_motion(
    SfCompanionState *companion, SfCompanionMotion motion) {
  if (companion->motion == motion) return;
  companion->motion = motion;
  companion->action_counter = 0u;
  sf_route_reset(&companion->route);
}

bool sf_companion_init(
    SfCompanionState *companion, const SfCompanionProfile *profile,
    SfWorldPoint position, uint8_t direction, bool defeated) {
  if (!companion || !profile || profile->type < 0 ||
      profile->type >= (int32_t) SF_COMPANION_COUNT ||
      profile->resource_id < 0 || profile->values[3] < 1 || direction >= 8u)
    return false;
  memset(companion, 0, sizeof(*companion));
  companion->profile = *profile;
  companion->position = position;
  companion->previous_position = position;
  companion->judgement = (SfObjectBounds) {-80, -80, 79, 79};
  companion->current_life = defeated ? 0 : profile->values[3];
  companion->damage.action = 2;
  companion->direction = direction;
  companion->inactive = true;
  companion->valid = true;
  sf_route_reset(&companion->route);
  return true;
}

void sf_companion_relocate(
    SfCompanionState *companion, SfWorldPoint position, uint8_t direction) {
  if (!companion || !companion->valid || direction >= 8u) return;
  companion->position = position;
  companion->previous_position = position;
  companion->direction = direction;
  companion->motion = SF_COMPANION_IDLE;
  companion->action_counter = 0u;
  companion->animation_frame = 0u;
  companion->close_linger_updates = 0u;
  sf_route_reset(&companion->route);
}

bool sf_companion_bind_profile(
    SfCompanionState *companion, const SfCompanionProfile *profile,
    SfWorldPoint position, uint8_t direction, bool defeated) {
  if (!companion || !profile) return false;
  if (!companion->valid)
    return sf_companion_init(
      companion, profile, position, direction, defeated);
  if (profile->type != companion->profile.type)
    return sf_companion_init(
      companion, profile, position, direction, defeated);
  companion->profile = *profile;
  if (companion->current_life > profile->values[3])
    companion->current_life = profile->values[3];
  sf_companion_relocate(companion, position, direction);
  return true;
}

void sf_companion_toggle_activity(SfCompanionState *companion) {
  if (!companion || !companion->valid) return;
  companion->inactive = !companion->inactive;
}

void sf_companion_update_follow(
    SfCompanionState *companion, const SfPlayerState *owner,
    const SfCollisionQuery *collision) {
  SfRouteStep movement;
  int distance;
  int32_t speed;
  if (!companion || !companion->valid || !owner || !collision ||
      companion->current_life <= 0) return;
  companion->previous_position = companion->position;
  distance = sf_movement_bounds_distance(
    companion->position, companion->judgement,
    owner->position, owner->judgement);
  if (distance >= SF_COMPANION_RELOCATE_DISTANCE) {
    companion->position.x = owner->position.x + SF_COMPANION_RELOCATE_OFFSET;
    companion->position.y = owner->position.y + SF_COMPANION_RELOCATE_OFFSET;
    companion->previous_position = companion->position;
    sf_route_reset(&companion->route);
    ++companion->action_counter;
    companion->animation_frame = companion->action_counter;
    return;
  }
  if (distance < SF_COMPANION_CLOSE_DISTANCE) {
    sf_companion_motion(companion, SF_COMPANION_IDLE);
    companion->close_linger_updates = SF_COMPANION_CLOSE_LINGER;
    companion->animation_frame = companion->action_counter++;
    return;
  }
  if (distance < SF_COMPANION_RUN_DISTANCE) {
    if (companion->close_linger_updates > 0u) {
      --companion->close_linger_updates;
      companion->animation_frame = companion->action_counter++;
      return;
    }
    sf_companion_motion(companion, SF_COMPANION_WALKING);
  } else {
    sf_companion_motion(companion, SF_COMPANION_RUNNING);
  }
  speed = companion->profile.values[
    companion->motion == SF_COMPANION_RUNNING ? 2 : 1] / 5;
  if (speed < 1) speed = 1;
  movement = sf_route_advance_query(
    &companion->route, collision, companion->judgement,
    companion->position, owner->position, (uint32_t) speed);
  if (movement.moved) {
    companion->direction = sf_movement_direction(
      companion->position, movement.position);
    companion->position = movement.position;
  }
  companion->animation_frame = companion->action_counter++;
}

SfWorldPoint sf_companion_render_position(
    const SfCompanionState *companion, uint16_t interpolation) {
  if (!companion) return (SfWorldPoint) {0, 0};
  return sf_world_point_interpolate(
    companion->previous_position, companion->position, interpolation);
}
