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

#include "game/world_transport.h"

#include "data/mct.h"

#include <limits.h>

bool sf_world_transport_activate(
    SfWorldState *world, int32_t destination_index) {
  const SfTransportDestination *destination;
  const SfMctEntry *entry;
  int32_t entry_key;
  if (!world || !world->transports || !world->scenario ||
      destination_index < 0 ||
      destination_index >= world->transports->count ||
      destination_index >= world->actor_script_state.progress.transport_count ||
      world->actor_script_state.progress
        .transport_values[destination_index] == 0) return false;
  destination = &world->transports->destinations[destination_index];
  if (destination->scenario_id != world->scenario_id ||
      destination->entry_value < 0 ||
      destination->entry_value > INT32_MAX / 4) return false;
  entry_key = destination->entry_value * 4;
  entry = sf_mct_find_entry(world->scenario, entry_key);
  if (!entry || entry->direction < 0 || entry->direction > 7) return false;
  world->entry_key = entry_key;
  sf_world_state_enter(
    world, entry->world_x, entry->world_y, (uint8_t) entry->direction);
  return true;
}
