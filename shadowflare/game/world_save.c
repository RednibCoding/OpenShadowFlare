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

#include "game/world_save.h"

#include <limits.h>
#include <string.h>

bool sf_world_prepare_save_load(
    SfWorldState *world, const SfSavedGame *saved) {
  int32_t mines;
  if (!world || !saved) return false;
  if (saved->world.present) {
    if (saved->world.scenario_id < 0 || saved->world.entry_value < 0 ||
        saved->world.entry_value > INT32_MAX / 4) return false;
    world->scenario_id = saved->world.scenario_id;
    world->entry_key = saved->world.entry_value * 4;
  }
  mines = saved->world.mine_count;
  if (mines < 0) mines = 0;
  if (mines > world->player.maximum_mines)
    mines = world->player.maximum_mines;
  world->player.mine_count = mines;
  world->player.pace = saved->world.running
    ? SF_PLAYER_PACE_RUN : SF_PLAYER_PACE_WALK;
  return true;
}

bool sf_world_bind_saved_scenario(
    SfWorldState *world,
    const SfMctScenario *scenario, const SfScsScript *script,
    const SfSavedGame *saved) {
  SfScenarioProgressState progress;
  if (!world || !scenario || !script || !saved) return false;
  if (!saved->progress.present)
    return sf_world_state_bind_scenario(world, scenario, script);
  memset(&progress, 0, sizeof(progress));
  memcpy(
    progress.persistent_values, saved->progress.script_values,
    sizeof(progress.persistent_values));
  memcpy(
    progress.quest_values, saved->progress.quest_values,
    sizeof(progress.quest_values));
  memcpy(
    progress.transport_values, saved->progress.transport_values,
    sizeof(progress.transport_values));
  progress.persistent_count = saved->progress.script_count;
  progress.quest_count = saved->progress.quest_count;
  progress.transport_count = saved->progress.transport_count;
  return sf_world_state_bind_scenario_progress(
    world, scenario, script, &progress);
}
