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

#include "render/depth.h"

#include <stdbool.h>

int sf_depth_class(int16_t status) {
  int result = (status & 0x100) != 0 ? 1 : 0;
  if ((status & 0x80) != 0) result = 2;
  if ((status & 0x20) != 0) result = 3;
  return result;
}

static int32_t sf_depth_top(const SfDepthEntry *entry) {
  SfWorldPoint point;
  point.x = entry->position.x + entry->judgement.left;
  point.y = entry->position.y + entry->judgement.top;
  return sf_world_to_screen(point).y;
}

static bool sf_depth_before(
    const SfDepthEntry *left, const SfDepthEntry *right) {
  const int left_class = sf_depth_class(left->status);
  const int right_class = sf_depth_class(right->status);
  if (left_class != right_class) return left_class < right_class;
  return sf_depth_top(left) < sf_depth_top(right);
}

static bool sf_depth_blocks(
    const SfDepthEntry *other, const SfDepthEntry *candidate) {
  const int other_class = sf_depth_class(other->status);
  const int candidate_class = sf_depth_class(candidate->status);
  const int32_t candidate_right =
    candidate->position.x + candidate->judgement.right;
  const int32_t candidate_bottom =
    candidate->position.y + candidate->judgement.bottom;
  const int32_t other_left = other->position.x + other->judgement.left;
  const int32_t other_top = other->position.y + other->judgement.top;
  const int32_t other_right = other->position.x + other->judgement.right;
  const int32_t other_bottom = other->position.y + other->judgement.bottom;
  if (other_class != candidate_class) return candidate_class < other_class;
  return other_left < candidate_right && other_top < candidate_bottom &&
    (other_right < candidate_right || other_bottom < candidate_bottom);
}

void sf_depth_sort(SfDepthEntry *entries, size_t count) {
  size_t index;
  if (!entries) return;
  for (index = 1u; index < count; ++index) {
    const SfDepthEntry selected = entries[index];
    size_t position = index;
    while (position > 0u &&
           sf_depth_before(&selected, &entries[position - 1u])) {
      entries[position] = entries[position - 1u];
      --position;
    }
    entries[position] = selected;
  }
  for (index = 0u; index < count; ++index) {
    size_t candidate;
    for (candidate = index; candidate < count; ++candidate) {
      size_t other;
      bool blocked = false;
      for (other = index; other < count; ++other) {
        if (other != candidate &&
            sf_depth_blocks(&entries[other], &entries[candidate])) {
          blocked = true;
          break;
        }
      }
      if (!blocked) break;
    }
    if (candidate > index && candidate < count) {
      const SfDepthEntry selected = entries[candidate];
      size_t move = candidate;
      while (move > index) {
        entries[move] = entries[move - 1u];
        --move;
      }
      entries[index] = selected;
    }
  }
}
