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

#ifndef SHADOWFLARE_GAME_COMBAT_EFFECT_REQUEST_H
#define SHADOWFLARE_GAME_COMBAT_EFFECT_REQUEST_H

#include "core/bounds.h"
#include "core/coordinates.h"
#include "game/combat_packet.h"

#include <stdbool.h>
#include <stdint.h>

#define SF_COMBAT_EFFECT_REQUEST_LIMIT 8u

typedef enum SfCombatEffectConstructorIndex {
  SF_COMBAT_EFFECT_CONSTRUCTOR_16 = 0,
  SF_COMBAT_EFFECT_CONSTRUCTOR_17,
  SF_COMBAT_EFFECT_CONSTRUCTOR_18,
  SF_COMBAT_EFFECT_CONSTRUCTOR_19,
  SF_COMBAT_EFFECT_CONSTRUCTOR_20,
  SF_COMBAT_EFFECT_CONSTRUCTOR_21,
  SF_COMBAT_EFFECT_CONSTRUCTOR_22
} SfCombatEffectConstructorIndex;

typedef struct SfCombatEffectRequest {
  SfCombatPacket packet;
  SfWorldPoint direction_vector;
  SfWorldPoint origin;
  SfObjectBounds source_judgement;
  int32_t effect_number;
  int32_t owner_kind;
  int32_t source_character_number;
  int32_t target_kind;
  int32_t target_identifier;
  int32_t constructor_value_6;
  int32_t constructor_value_7;
  int32_t constructor_value_12;
  int32_t packet_kind;
  int32_t instance_identifier;
  int32_t constructor_values_16_to_22[7];
  bool valid;
  bool has_explicit_origin;
  bool has_source_judgement;
  bool has_packet;
} SfCombatEffectRequest;

typedef struct SfCombatEffectRequestQueue {
  SfCombatEffectRequest requests[SF_COMBAT_EFFECT_REQUEST_LIMIT];
  uint8_t count;
  bool overflowed;
} SfCombatEffectRequestQueue;

void sf_combat_effect_requests_reset(SfCombatEffectRequestQueue *queue);
bool sf_combat_effect_requests_push(
  SfCombatEffectRequestQueue *queue, const SfCombatEffectRequest *request);

#endif
