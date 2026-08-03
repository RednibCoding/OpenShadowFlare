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

#include "runtime/profiler.h"

#include <string.h>

void sf_profiler_init(SfRuntimeProfiler *profiler, uint64_t now_us) {
  if (!profiler) return;
  memset(profiler, 0, sizeof(*profiler));
  profiler->window_started_us = now_us;
}

void sf_profiler_record_frame(
    SfRuntimeProfiler *profiler, uint64_t now_us,
    uint32_t fill_us, uint32_t present_us,
    size_t main_bytes, size_t video_bytes) {
  uint64_t elapsed;
  if (!profiler) return;
  profiler->snapshot.main_bytes = main_bytes;
  profiler->snapshot.video_bytes = video_bytes;
  if (main_bytes > profiler->snapshot.main_peak_bytes)
    profiler->snapshot.main_peak_bytes = main_bytes;
  if (video_bytes > profiler->snapshot.video_peak_bytes)
    profiler->snapshot.video_peak_bytes = video_bytes;
  if (fill_us > profiler->snapshot.fill_peak_us)
    profiler->snapshot.fill_peak_us = fill_us;
  if (present_us > profiler->snapshot.present_peak_us)
    profiler->snapshot.present_peak_us = present_us;
  profiler->fill_total_us += fill_us;
  profiler->present_total_us += present_us;
  ++profiler->window_frames;
  elapsed = now_us - profiler->window_started_us;
  if (elapsed < UINT64_C(1000000) || profiler->window_frames == 0u) return;
  profiler->snapshot.frames_per_second = (uint16_t) (
    (uint64_t) profiler->window_frames * UINT64_C(1000000) / elapsed);
  profiler->snapshot.fill_average_us = (uint32_t) (
    profiler->fill_total_us / profiler->window_frames);
  profiler->snapshot.present_average_us = (uint32_t) (
    profiler->present_total_us / profiler->window_frames);
  profiler->snapshot.ready = true;
  profiler->window_started_us = now_us;
  profiler->fill_total_us = 0u;
  profiler->present_total_us = 0u;
  profiler->window_frames = 0u;
}

const SfProfileSnapshot *sf_profiler_snapshot(
    const SfRuntimeProfiler *profiler) {
  return profiler ? &profiler->snapshot : NULL;
}
