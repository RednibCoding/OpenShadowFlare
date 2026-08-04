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

#ifndef SHADOWFLARE_GAME_SCENARIO_ACTOR_H
#define SHADOWFLARE_GAME_SCENARIO_ACTOR_H

#include "core/bounds.h"
#include "core/coordinates.h"
#include "data/mct.h"
#include "game/route.h"
#include "game/scenario_entity.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct SfScenarioActor {
  SfWorldPoint position;
  SfWorldPoint previous_position;
  SfWorldPoint destination;
  SfWorldPoint wander_min;
  SfWorldPoint wander_max;
  SfObjectBounds judgement;
  SfRouteController route;
  int32_t id;
  int32_t resource_id;
  int32_t state[SF_MCT_ENTITY_STATE_COUNT];
  int16_t red_strength[SF_MCT_PERSON_PART_LIMIT];
  int16_t green_strength[SF_MCT_PERSON_PART_LIMIT];
  int16_t blue_strength[SF_MCT_PERSON_PART_LIMIT];
  uint32_t random_state;
  uint32_t animation_frame;
  uint16_t action_counter;
  uint16_t walk_speed;
  uint16_t walk_duration;
  uint16_t idle_duration;
  uint8_t animation_chart;
  uint8_t direction;
  uint8_t enabled_parts;
  bool wandering_enabled;
  bool walking;
  bool scripted_turning_enabled;
  bool interaction_active;
} SfScenarioActor;

typedef struct SfScenarioActorSet {
  SfScenarioActor actors[SF_MCT_PERSON_LIMIT];
  uint8_t count;
} SfScenarioActorSet;

void sf_scenario_actors_init(
  SfScenarioActorSet *actors, const SfMctScenario *scenario);
void sf_scenario_actor_update(
  SfScenarioActor *actor, const SfCollisionQuery *collision);
int32_t sf_scenario_actor_character_number(const SfScenarioActor *actor);
SfWorldPoint sf_scenario_actor_render_position(
  const SfScenarioActor *actor, uint16_t interpolation);
void sf_scenario_actor_begin_interaction(SfScenarioActor *actor);
void sf_scenario_actor_face_toward(
  SfScenarioActor *actor, SfWorldPoint target);
void sf_scenario_actor_release_interaction(SfScenarioActor *actor);
SfScenarioActor *sf_scenario_actor_find(
  SfScenarioActorSet *actors, int32_t character_number);
const SfScenarioActor *sf_scenario_actor_find_const(
  const SfScenarioActorSet *actors, int32_t character_number);
const SfScenarioActor *sf_scenario_actor_at(
  const SfScenarioActorSet *actors, uint8_t index);
bool sf_scenario_actor_state(
  const SfScenarioActor *actor, SfScenarioEntityChannel channel);
void sf_scenario_actor_set_state(
  SfScenarioActor *actor, SfScenarioEntityChannel channel, int32_t value);

#endif
