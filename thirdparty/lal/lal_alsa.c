/*
 * Copyright (C) 2026 Michael Binder and contributors
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

#define _POSIX_C_SOURCE 200809L

#include "lal_internal.h"

#include <alsa/asoundlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

static snd_pcm_t *g_pcm;
static pthread_t g_thread;
static pthread_mutex_t g_mutex;
static bool g_mutex_ready;
static bool g_running;

static bool lal_alsa_is_running(void) {
  bool running;

  pthread_mutex_lock(&g_mutex);
  running = g_running;
  pthread_mutex_unlock(&g_mutex);
  return running;
}

static void *lal_alsa_thread(void *unused) {
  int16_t samples[LAL_BUFFER_FRAMES * LAL_OUTPUT_CHANNELS];

  (void) unused;
  while (lal_alsa_is_running()) {
    snd_pcm_sframes_t remaining;
    int16_t *next_sample;

    pthread_mutex_lock(&g_mutex);
    lal_mix_frames(samples, LAL_BUFFER_FRAMES);
    pthread_mutex_unlock(&g_mutex);

    remaining = LAL_BUFFER_FRAMES;
    next_sample = samples;
    while (remaining > 0 && lal_alsa_is_running()) {
      snd_pcm_sframes_t written;

      written = snd_pcm_writei(g_pcm, next_sample, (snd_pcm_uframes_t) remaining);
      if (written < 0) {
        if (snd_pcm_recover(g_pcm, (int) written, 1) < 0) {
          break;
        }
        continue;
      }
      if (written == 0) {
        break;
      }

      remaining -= written;
      next_sample += written * LAL_OUTPUT_CHANNELS;
    }
  }

  return NULL;
}

bool lal_platform_init(void) {
  int error;

  if (pthread_mutex_init(&g_mutex, NULL) != 0) {
    lal_set_error("Could not create the ALSA mixer mutex.");
    return false;
  }
  g_mutex_ready = true;

  error = snd_pcm_open(&g_pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
  if (error < 0) {
    lal_set_error("Could not open the default ALSA playback device.");
    pthread_mutex_destroy(&g_mutex);
    g_mutex_ready = false;
    return false;
  }

  error = snd_pcm_set_params(
    g_pcm,
    SND_PCM_FORMAT_S16_LE,
    SND_PCM_ACCESS_RW_INTERLEAVED,
    LAL_OUTPUT_CHANNELS,
    LAL_OUTPUT_SAMPLE_RATE,
    1,
    50000);
  if (error < 0) {
    lal_set_error("Could not configure the default ALSA playback device.");
    snd_pcm_close(g_pcm);
    g_pcm = NULL;
    pthread_mutex_destroy(&g_mutex);
    g_mutex_ready = false;
    return false;
  }

  g_running = true;
  error = pthread_create(&g_thread, NULL, lal_alsa_thread, NULL);
  if (error != 0) {
    lal_set_error("Could not create the ALSA playback thread.");
    g_running = false;
    snd_pcm_close(g_pcm);
    g_pcm = NULL;
    pthread_mutex_destroy(&g_mutex);
    g_mutex_ready = false;
    return false;
  }

  return true;
}

void lal_platform_shutdown(void) {
  pthread_mutex_lock(&g_mutex);
  g_running = false;
  pthread_mutex_unlock(&g_mutex);

  snd_pcm_drop(g_pcm);
  pthread_join(g_thread, NULL);
  snd_pcm_close(g_pcm);
  g_pcm = NULL;

  pthread_mutex_destroy(&g_mutex);
  g_mutex_ready = false;
}

void lal_platform_lock(void) {
  if (g_mutex_ready) {
    pthread_mutex_lock(&g_mutex);
  }
}

void lal_platform_unlock(void) {
  if (g_mutex_ready) {
    pthread_mutex_unlock(&g_mutex);
  }
}
