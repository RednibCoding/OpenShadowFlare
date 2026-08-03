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

#include "core/retail_random.h"
#include "game/movement.h"

#include <limits.h>
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
    actor->destination = actor->position;
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
    actor->walk_speed = person->walk_speed < 0 ? 0u :
      person->walk_speed > UINT16_MAX ? UINT16_MAX :
      (uint16_t) person->walk_speed;
    actor->walk_duration = person->walk_duration < 0 ? 0u :
      person->walk_duration > UINT16_MAX ? UINT16_MAX :
      (uint16_t) person->walk_duration;
    actor->idle_duration = person->idle_duration < 0 ? 0u :
      person->idle_duration > UINT16_MAX ? UINT16_MAX :
      (uint16_t) person->idle_duration;
    actor->wander_min.x = person->wander_left;
    actor->wander_min.y = person->wander_top;
    actor->wander_max.x = person->wander_right;
    actor->wander_max.y = person->wander_bottom;
    if (person->wander_bounds_relative) {
      actor->wander_min.x += actor->position.x;
      actor->wander_min.y += actor->position.y;
      actor->wander_max.x += actor->position.x;
      actor->wander_max.y += actor->position.y;
    }
    if (actor->wander_min.x > actor->wander_max.x) {
      const int32_t value = actor->wander_min.x;
      actor->wander_min.x = actor->wander_max.x;
      actor->wander_max.x = value;
    }
    if (actor->wander_min.y > actor->wander_max.y) {
      const int32_t value = actor->wander_min.y;
      actor->wander_min.y = actor->wander_max.y;
      actor->wander_max.y = value;
    }
    actor->wandering_enabled = person->wandering_enabled &&
      actor->walk_speed > 0u && actor->walk_duration > 0u;
    actor->random_state = (uint32_t) person->id + 1u;
    sf_route_reset(&actor->route);
    for (part = 0u; part < SF_MCT_PERSON_PART_LIMIT; ++part) {
      if (!person->custom_parts || person->part_visibility[part] != 0u)
        actor->enabled_parts = (uint8_t) (
          actor->enabled_parts | (uint8_t) (1u << part));
    }
  }
}

static int32_t sf_scenario_actor_random_coordinate(
    SfScenarioActor *actor, int32_t first, int32_t last) {
  const uint32_t span = (uint32_t) ((int64_t) last - first + 1);
  return first + (int32_t) (
    sf_retail_random_next(&actor->random_state) % span);
}

void sf_scenario_actor_update(
    SfScenarioActor *actor, const SfCollisionQuery *collision) {
  SfRouteStep movement;
  if (!actor) return;
  actor->previous_position = actor->position;
  if (!actor->walking) {
    actor->animation_chart = 0u;
    actor->animation_frame = actor->action_counter;
    if (!actor->wandering_enabled ||
        actor->action_counter++ < actor->idle_duration) return;
    actor->destination.x = sf_scenario_actor_random_coordinate(
      actor, actor->wander_min.x, actor->wander_max.x);
    actor->destination.y = sf_scenario_actor_random_coordinate(
      actor, actor->wander_min.y, actor->wander_max.y);
    actor->walking = actor->destination.x != actor->position.x ||
      actor->destination.y != actor->position.y;
    actor->action_counter = 0u;
    sf_route_reset(&actor->route);
    if (!actor->walking) return;
  }
  actor->animation_chart = 1u;
  actor->animation_frame = actor->action_counter;
  actor->direction = sf_movement_direction(
    actor->position, actor->destination);
  movement = sf_route_advance_query(
    &actor->route, collision, actor->judgement,
    actor->position, actor->destination, actor->walk_speed);
  if (movement.moved)
    actor->direction = sf_movement_direction(
      actor->position, movement.position);
  actor->position = movement.position;
  ++actor->action_counter;
  if ((!movement.moved && !movement.controller_active) ||
      (actor->position.x == actor->destination.x &&
       actor->position.y == actor->destination.y) ||
      actor->action_counter >= actor->walk_duration) {
    actor->walking = false;
    actor->destination = actor->position;
    actor->action_counter = 0u;
  }
}

int32_t sf_scenario_actor_character_number(const SfScenarioActor *actor) {
  return actor ? SF_SCENARIO_PERSON_CHARACTER_BASE + actor->id : INT32_MIN;
}

SfWorldPoint sf_scenario_actor_render_position(
    const SfScenarioActor *actor, uint16_t interpolation) {
  if (!actor) return (SfWorldPoint) {0, 0};
  return sf_world_point_interpolate(
    actor->previous_position, actor->position, interpolation);
}

SfScenarioActor *sf_scenario_actor_find(
    SfScenarioActorSet *actors, int32_t character_number) {
  uint8_t index;
  if (!actors) return NULL;
  for (index = 0u; index < actors->count; ++index) {
    if (sf_scenario_actor_character_number(&actors->actors[index]) ==
        character_number) return &actors->actors[index];
  }
  return NULL;
}

const SfScenarioActor *sf_scenario_actor_find_const(
    const SfScenarioActorSet *actors, int32_t character_number) {
  uint8_t index;
  if (!actors) return NULL;
  for (index = 0u; index < actors->count; ++index) {
    if (sf_scenario_actor_character_number(&actors->actors[index]) ==
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
