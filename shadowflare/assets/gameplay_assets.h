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

#ifndef SHADOWFLARE_ASSETS_GAMEPLAY_ASSETS_H
#define SHADOWFLARE_ASSETS_GAMEPLAY_ASSETS_H

#include "assets/player_assets.h"
#include "assets/ground_item_assets.h"
#include "assets/scenario_actor_assets.h"
#include "core/arena.h"
#include "data/gnd.h"
#include "data/mct.h"
#include "data/njp.h"
#include "data/obl.h"
#include "data/scs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SF_GAMEPLAY_PATTERN_SET_LIMIT 16u

typedef struct SfGameplayPatternSet {
  SfNjpDecodedResource resource;
  uint8_t source_index;
} SfGameplayPatternSet;

typedef struct SfGameplayAssets {
  SfGroundMap ground;
  SfObjectMap objects;
  SfMctScenario scenario;
  SfScsScript *script;
  SfMctEntry entry;
  SfNjpSelected font;
  SfNjpSelected speech_frame;
  SfPlayerAssets player;
  SfScenarioActorAssets actors;
  SfGroundItemAssets ground_items;
  SfGameplayPatternSet *pattern_sets;
  size_t memory_bytes;
  uint8_t pattern_set_count;
} SfGameplayAssets;

bool sf_gameplay_assets_load(
  SfGameplayAssets *assets, const char *data_root,
  int32_t scenario_id, int32_t entry_key, uint8_t player_gender,
  const uint8_t *appearance_parts, uint8_t appearance_part_count,
  const SfItemReference *visible_items, uint8_t visible_item_count,
  SfArena *arena);
const SfNjpDecodedResource *sf_gameplay_pattern_set(
  const SfGameplayAssets *assets, uint8_t source_index);

#endif
