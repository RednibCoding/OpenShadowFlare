/*
 * Copyright (C) 2026 Michael Binder
 *
 * This file is part of LAL.
 *
 * LAL is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * LAL is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along with
 * LAL. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LAL_INTERNAL_H
#define LAL_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
  LAL_OUTPUT_SAMPLE_RATE = 44100,
  LAL_OUTPUT_CHANNELS = 2,
  LAL_MAX_VOICES = 32,
  LAL_BUFFER_FRAMES = 1024
};

bool lal_platform_init(void);
void lal_platform_shutdown(void);
void lal_platform_lock(void);
void lal_platform_unlock(void);

/* Called by the platform audio callback while the platform lock is held. */
void lal_mix_frames(int16_t *output, size_t frame_count);

void lal_set_error(const char *message);

#endif
