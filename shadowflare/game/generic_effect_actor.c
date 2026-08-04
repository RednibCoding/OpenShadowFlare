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

#include "game/generic_effect_actor.h"

#include "game/movement.h"

#include <limits.h>
#include <string.h>

static int32_t sf_generic_effect_resource(int32_t effect_number) {
  static const int32_t resources[8] = {0, 1, 2, 3, 0, 0, 4, 0};
  return effect_number >= 0 && effect_number < 8
    ? resources[effect_number] : -1;
}

static SfObjectBounds sf_generic_effect_judgement(int32_t effect_number) {
  if (effect_number == 6)
    return (SfObjectBounds) {-160, -160, 159, 159};
  return (SfObjectBounds) {-30, -30, 30, 30};
}

static bool sf_generic_effect_position(
    const SfCombatEffectRequest *request, SfWorldPoint resolved_source,
    SfWorldPoint *position) {
  SfWorldPoint offset;
  int64_t x;
  int64_t y;
  if (request->owner_kind == 0 && request->has_explicit_origin) {
    *position = request->origin;
    return true;
  }
  if (request->constructor_values_16_to_22[
        SF_COMBAT_EFFECT_CONSTRUCTOR_21] < 0) return false;
  if (!sf_movement_vector_at_distance(
        request->direction_vector,
        (uint32_t) request->constructor_values_16_to_22[
          SF_COMBAT_EFFECT_CONSTRUCTOR_21], &offset)) return false;
  x = (int64_t) resolved_source.x + offset.x;
  y = (int64_t) resolved_source.y + offset.y;
  if (x < INT32_MIN || x > INT32_MAX ||
      y < INT32_MIN || y > INT32_MAX) return false;
  *position = (SfWorldPoint) {(int32_t) x, (int32_t) y};
  return true;
}

bool sf_generic_effect_actor_build(
    const SfCombatEffectRequest *request, SfWorldPoint resolved_source,
    SfGenericEffectActor *actor) {
  const SfWorldPoint zero = {0, 0};
  int32_t resource;
  if (!actor) return false;
  memset(actor, 0, sizeof(*actor));
  if (!request || !request->valid) return false;
  resource = sf_generic_effect_resource(request->effect_number);
  if (resource < 0 || request->effect_number == 2) return false;
  if (!sf_generic_effect_position(request, resolved_source, &actor->position))
    return false;
  actor->packet = request->packet;
  actor->direction_vector = request->direction_vector;
  actor->judgement = sf_generic_effect_judgement(request->effect_number);
  actor->effect_number = request->effect_number;
  actor->resource_id = resource;
  actor->owner_kind = request->owner_kind;
  actor->source_character_number = request->source_character_number;
  actor->target_kind = request->target_kind;
  actor->target_identifier = request->target_identifier;
  actor->travel_speed = request->constructor_value_6;
  actor->display_height = request->constructor_value_7;
  actor->lifetime = -1;
  actor->target_collision_start = 0;
  actor->contact_effect_number = request->effect_number == 6 ? 21023 : -1;
  actor->animation_chart = 0;
  actor->animation_direction = request->effect_number == 1 ? 8 :
    request->packet_kind == 8
      ? sf_movement_direction(zero, request->direction_vector)
      : request->packet_kind;
  actor->target_audio_sample = 20u;
  actor->valid = true;
  actor->home_toward_target =
    request->constructor_values_16_to_22[
      SF_COMBAT_EFFECT_CONSTRUCTOR_20] == 1 && request->effect_number != 1;
  actor->collide_with_environment = true;
  actor->expire_on_environment_collision = true;
  actor->expire_on_target = true;
  actor->remember_targets = request->constructor_values_16_to_22[
    SF_COMBAT_EFFECT_CONSTRUCTOR_22] == 1;
  actor->has_packet = request->has_packet;
  return true;
}
