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

#ifndef SHADOWFLARE_GAME_SOUND_EVENT_H
#define SHADOWFLARE_GAME_SOUND_EVENT_H

#include <stdint.h>

#define SF_SOUND_EVENT_LIMIT 8u

typedef struct SfSoundEventQueue {
  uint16_t samples[SF_SOUND_EVENT_LIMIT];
  uint8_t count;
} SfSoundEventQueue;

void sf_sound_events_reset(SfSoundEventQueue *events);
void sf_sound_events_push(SfSoundEventQueue *events, uint16_t sample);

#endif
