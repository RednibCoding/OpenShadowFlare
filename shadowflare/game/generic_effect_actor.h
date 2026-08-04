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

#ifndef SHADOWFLARE_GAME_GENERIC_EFFECT_ACTOR_H
#define SHADOWFLARE_GAME_GENERIC_EFFECT_ACTOR_H

#include "core/bounds.h"
#include "core/coordinates.h"
#include "game/combat_effect_request.h"
#include "game/combat_packet.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct SfGenericEffectActor {
  SfCombatPacket packet;
  SfWorldPoint position;
  SfWorldPoint direction_vector;
  SfObjectBounds judgement;
  int32_t effect_number;
  int32_t resource_id;
  int32_t owner_kind;
  int32_t source_character_number;
  int32_t target_kind;
  int32_t target_identifier;
  int32_t travel_speed;
  int32_t display_height;
  int32_t lifetime;
  int32_t target_collision_start;
  int32_t contact_effect_number;
  int32_t animation_chart;
  int32_t animation_direction;
  uint16_t target_audio_sample;
  bool valid;
  bool home_toward_target;
  bool collide_with_environment;
  bool expire_on_environment_collision;
  bool expire_on_target;
  bool remember_targets;
  bool has_packet;
} SfGenericEffectActor;

bool sf_generic_effect_actor_build(
  const SfCombatEffectRequest *request, SfWorldPoint resolved_source,
  SfGenericEffectActor *actor);

#endif
