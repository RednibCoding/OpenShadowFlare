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

#ifndef SHADOWFLARE_RUNTIME_PROFILER_H
#define SHADOWFLARE_RUNTIME_PROFILER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct SfProfileSnapshot {
  size_t main_bytes;
  size_t main_peak_bytes;
  size_t video_bytes;
  size_t video_peak_bytes;
  uint32_t fill_average_us;
  uint32_t fill_peak_us;
  /* Surface preparation only; display, swap, and vblank wait are excluded. */
  uint32_t present_average_us;
  uint32_t present_peak_us;
  uint16_t frames_per_second;
  bool ready;
} SfProfileSnapshot;

typedef struct SfRuntimeProfiler {
  SfProfileSnapshot snapshot;
  uint64_t window_started_us;
  uint64_t fill_total_us;
  uint64_t present_total_us;
  uint32_t window_frames;
} SfRuntimeProfiler;

void sf_profiler_init(SfRuntimeProfiler *profiler, uint64_t now_us);
void sf_profiler_record_frame(
  SfRuntimeProfiler *profiler, uint64_t now_us,
  uint32_t fill_us, uint32_t present_us,
  size_t main_bytes, size_t video_bytes);
const SfProfileSnapshot *sf_profiler_snapshot(
  const SfRuntimeProfiler *profiler);

#endif
