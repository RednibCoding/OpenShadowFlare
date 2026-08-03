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

#include "core/memory_budget.h"
#include "runtime/application.h"

#include <stdint.h>

typedef union SfAlignedMainMemory {
  long double floating_point;
  void *pointer;
  uint64_t integer;
  uint8_t bytes[SF_MAIN_ARENA_BYTES];
} SfAlignedMainMemory;

typedef union SfAlignedVideoMemory {
  long double floating_point;
  void *pointer;
  uint64_t integer;
  uint8_t bytes[SF_VIDEO_MEMORY_LIMIT_BYTES];
} SfAlignedVideoMemory;

static SfAlignedMainMemory sf_main_memory;
static SfAlignedVideoMemory sf_video_memory;

int main(int argument_count, char **arguments) {
  return sf_application_run(
    sf_main_memory.bytes, sizeof(sf_main_memory.bytes),
    sf_video_memory.bytes, sizeof(sf_video_memory.bytes),
    argument_count > 0 ? arguments[0] : NULL,
    argument_count > 1 ? arguments[1] : NULL);
}
