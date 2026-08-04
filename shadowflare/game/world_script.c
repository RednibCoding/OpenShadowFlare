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
    world};
  return environment;
}
