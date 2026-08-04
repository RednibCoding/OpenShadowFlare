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

#include <stdio.h>

int main(void) {
  SfRuntimeProfiler profiler;
  const SfProfileSnapshot *snapshot;
  unsigned frame;
  sf_profiler_init(&profiler, 0u);
  for (frame = 0u; frame < 50u; ++frame)
    sf_profiler_record_frame(
      &profiler, (uint64_t) (frame + 1u) * UINT64_C(20000),
      150u, 50u, 1000u + frame, 600u);
  snapshot = sf_profiler_snapshot(&profiler);
  if (!snapshot || !snapshot->ready ||
      snapshot->frames_per_second != 50u ||
      snapshot->fill_average_us != 150u ||
      snapshot->present_average_us != 50u ||
      snapshot->fill_peak_us != 150u ||
      snapshot->present_peak_us != 50u ||
      snapshot->main_bytes != 1049u ||
      snapshot->main_peak_bytes != 1049u ||
      snapshot->video_bytes != 600u ||
      snapshot->video_peak_bytes != 600u) {
    fputs("the C99 runtime profiler produced the wrong snapshot\n", stderr);
    return 1;
  }
  return 0;
}
