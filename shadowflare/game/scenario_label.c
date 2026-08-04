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

#include "game/scenario_label.h"

#define SF_SCENARIO_LABEL_HASH_BASIS UINT32_C(2166136261)
#define SF_SCENARIO_LABEL_HASH_PRIME UINT32_C(16777619)

static uint32_t sf_scenario_label_hash(
    uint32_t hash, int32_t value) {
  uint8_t byte;
  for (byte = 0u; byte < 4u; ++byte) {
    hash ^= ((uint32_t) value >> (byte * 8u)) & 0xffu;
    hash *= SF_SCENARIO_LABEL_HASH_PRIME;
  }
  return hash;
}

void sf_scenario_labels_begin(SfScenarioLabelSet *labels) {
  if (!labels) return;
  labels->count = 0u;
  labels->building_signature = SF_SCENARIO_LABEL_HASH_BASIS;
  labels->building = true;
}

bool sf_scenario_labels_add(
    SfScenarioLabelSet *labels, SfScenarioLabel label) {
  uint32_t *hash;
  if (!labels || !labels->building ||
      labels->count >= SF_SCENARIO_LABEL_LIMIT) return false;
  labels->labels[labels->count++] = label;
  hash = &labels->building_signature;
  *hash = sf_scenario_label_hash(*hash, label.anchor.x);
  *hash = sf_scenario_label_hash(*hash, label.anchor.y);
  *hash = sf_scenario_label_hash(*hash, label.offset_x);
  *hash = sf_scenario_label_hash(*hash, label.offset_y);
  *hash = sf_scenario_label_hash(*hash, label.message_id);
  *hash = sf_scenario_label_hash(*hash, label.red);
  *hash = sf_scenario_label_hash(*hash, label.green);
  *hash = sf_scenario_label_hash(*hash, label.blue);
  *hash = sf_scenario_label_hash(*hash, label.background_opacity);
  return true;
}

void sf_scenario_labels_end(SfScenarioLabelSet *labels) {
  if (!labels || !labels->building) return;
  labels->building_signature = sf_scenario_label_hash(
    labels->building_signature, labels->count);
  if (labels->signature != labels->building_signature) {
    labels->signature = labels->building_signature;
    ++labels->revision;
  }
  labels->building = false;
}
