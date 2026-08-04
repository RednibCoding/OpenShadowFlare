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

#include "game/world_script.h"

#include "core/retail_random.h"

static bool sf_world_script_anchor(
    const SfWorldState *world, int32_t character_number,
    SfWorldPoint *anchor) {
  const SfScenarioActor *actor;
  const SfScenarioObject *object;
  if (!world || !anchor) return false;
  if (character_number >= 0 && character_number < 4) {
    if (character_number != 0) return false;
    *anchor = world->player.position;
    return true;
  }
  actor = sf_scenario_actor_find_const(&world->actors, character_number);
  if (actor) {
    *anchor = actor->position;
    return true;
  }
  object = sf_scenario_object_find_const(
    &world->scenario_objects, character_number);
  if (!object) return false;
  *anchor = object->position;
  return true;
}

static bool sf_world_native_command(
    void *user, int32_t opcode, const int32_t *arguments,
    uint8_t argument_count) {
  SfWorldState *world = (SfWorldState *) user;
  if (!world || !arguments) return false;
  if (opcode == 10 && argument_count == 6u)
    return sf_ground_items_create(
      &world->ground_items, arguments[0], arguments[1],
      (SfWorldPoint) {arguments[2], arguments[3]},
      arguments[4], arguments[5]);
  if (opcode == 16 && argument_count >= 1u && argument_count <= 4u) {
    if (arguments[0] < 0 || arguments[0] > UINT16_MAX) return false;
    if (argument_count >= 4u && arguments[1] == 0 &&
        sf_movement_point_distance(
          world->player.position,
          (SfWorldPoint) {arguments[2], arguments[3]}) > 3000) return true;
    sf_sound_events_push(&world->sounds, (uint16_t) arguments[0]);
    return true;
  }
  if (opcode == 27 && argument_count == 8u) {
    SfWorldPoint anchor;
    if (!world->script ||
        !sf_world_script_anchor(world, arguments[0], &anchor) ||
        !sf_scs_message(world->script, arguments[3])) return false;
    return sf_scenario_labels_add(
      &world->scenario_labels, (SfScenarioLabel) {
        anchor, arguments[1], arguments[2], arguments[3],
        arguments[4], arguments[5], arguments[6], arguments[7]});
  }
  if (opcode == 36)
    return sf_scenario_placed_effect_add(
      &world->placed_effects, arguments, argument_count);
  if (opcode == 37 && argument_count == 1u) {
    if (!sf_gameplay_service_request(
          &world->service_request,
          SF_GAMEPLAY_SERVICE_OPEN_TRANSPORT, arguments[0])) return false;
    world->script_transport_service = arguments[0];
    return true;
  }
  if (opcode == 38 && argument_count == 1u) {
    if (world->script_transport_service != arguments[0]) return true;
    if (!sf_gameplay_service_request(
          &world->service_request,
          SF_GAMEPLAY_SERVICE_CLOSE_TRANSPORT, arguments[0])) return false;
    world->script_transport_service = -1;
    return true;
  }
  if (opcode == 41 && argument_count == 1u)
    return sf_gameplay_service_request(
      &world->service_request,
      SF_GAMEPLAY_SERVICE_TOGGLE_SPECIAL_ITEMS, arguments[0]);
  if (opcode == 46 && argument_count == 2u) {
    SfScenarioObject *object = sf_scenario_object_find(
      &world->scenario_objects, arguments[0]);
    if (object) object->draw_strength = arguments[1];
    return true;
  }
  if (opcode == 56 && argument_count == 4u) {
    SfScenarioObject *object = sf_scenario_object_find(
      &world->scenario_objects, arguments[0]);
    if (object) sf_scenario_object_set_state_override(
      object, arguments[1], arguments[2], arguments[3]);
    return true;
  }
  return false;
}

static bool sf_world_next_random(void *user, int32_t *value) {
  SfWorldState *world = (SfWorldState *) user;
  if (!world || !value) return false;
  *value = sf_retail_random_next(&world->random_state);
  return true;
}

SfScenarioScriptEnvironment sf_world_script_environment(
    SfWorldState *world) {
  const SfScenarioScriptEnvironment environment = {
    world ? world->scenario : NULL,
    world ? &world->actors : NULL,
    world ? &world->scenario_objects : NULL,
    world ? world->player.position : (SfWorldPoint) {0, 0},
    world ? world->player.judgement : (SfObjectBounds) {0, 0, 0, 0},
    world ? world->companion_type : 0,
    sf_world_native_command,
    sf_world_next_random,
    world};
  return environment;
}
