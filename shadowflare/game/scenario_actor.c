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

#include "game/scenario_actor.h"

#include <string.h>

#define SF_SCENARIO_PERSON_CHARACTER_BASE 12000000

void sf_scenario_actors_init(
    SfScenarioActorSet *actors, const SfMctScenario *scenario) {
  uint8_t index;
  if (!actors) return;
  memset(actors, 0, sizeof(*actors));
  if (!scenario) return;
  actors->count = scenario->people_count;
  for (index = 0u; index < actors->count; ++index) {
    const SfMctPerson *person = &scenario->people[index];
    SfScenarioActor *actor = &actors->actors[index];
    uint8_t part;
    actor->id = person->id;
    actor->resource_id = person->resource_id;
    actor->position.x = person->world_x;
    actor->position.y = person->world_y;
    actor->previous_position = actor->position;
    actor->judgement.left = person->judgement_left;
    actor->judgement.top = person->judgement_top;
    actor->judgement.right = person->judgement_right;
    actor->judgement.bottom = person->judgement_bottom;
    actor->direction = (uint8_t) person->direction;
    memcpy(actor->state, person->initial_state, sizeof(actor->state));
    memcpy(actor->red_strength, person->red_strength,
      sizeof(actor->red_strength));
    memcpy(actor->green_strength, person->green_strength,
      sizeof(actor->green_strength));
    memcpy(actor->blue_strength, person->blue_strength,
      sizeof(actor->blue_strength));
    for (part = 0u; part < SF_MCT_PERSON_PART_LIMIT; ++part) {
      if (!person->custom_parts || person->part_visibility[part] != 0u)
        actor->enabled_parts = (uint8_t) (
          actor->enabled_parts | (uint8_t) (1u << part));
    }
  }
}

void sf_scenario_actors_update(SfScenarioActorSet *actors) {
  uint8_t index;
  if (!actors) return;
  for (index = 0u; index < actors->count; ++index) {
    SfScenarioActor *actor = &actors->actors[index];
    actor->previous_position = actor->position;
    ++actor->animation_frame;
  }
}

SfScenarioActor *sf_scenario_actor_find(
    SfScenarioActorSet *actors, int32_t character_number) {
  uint8_t index;
  if (!actors) return NULL;
  for (index = 0u; index < actors->count; ++index) {
    if (SF_SCENARIO_PERSON_CHARACTER_BASE + actors->actors[index].id ==
        character_number) return &actors->actors[index];
  }
  return NULL;
}

const SfScenarioActor *sf_scenario_actor_find_const(
    const SfScenarioActorSet *actors, int32_t character_number) {
  uint8_t index;
  if (!actors) return NULL;
  for (index = 0u; index < actors->count; ++index) {
    if (SF_SCENARIO_PERSON_CHARACTER_BASE + actors->actors[index].id ==
        character_number) return &actors->actors[index];
  }
  return NULL;
}

const SfScenarioActor *sf_scenario_actor_at(
    const SfScenarioActorSet *actors, uint8_t index) {
  return actors && index < actors->count ? &actors->actors[index] : NULL;
}

bool sf_scenario_actor_state(
    const SfScenarioActor *actor, SfScenarioEntityChannel channel) {
  return actor && channel < SF_MCT_ENTITY_STATE_COUNT &&
    actor->state[channel] != 0;
}

void sf_scenario_actor_set_state(
    SfScenarioActor *actor, SfScenarioEntityChannel channel, int32_t value) {
  if (actor && channel < SF_MCT_ENTITY_STATE_COUNT)
    actor->state[channel] = value;
}
