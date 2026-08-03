/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of LAL.
 *
 * LAL is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#include "lal_internal.h"

#include <switch.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum {
  LAL_SWITCH_BUFFER_COUNT = 3,
  LAL_SWITCH_OUTPUT_FRAMES = 960,
  LAL_SWITCH_INPUT_FRAMES = 882
};

static AudioOutBuffer g_buffers[LAL_SWITCH_BUFFER_COUNT];
static int16_t g_samples[LAL_SWITCH_BUFFER_COUNT]
    [0x1000 / sizeof(int16_t)]
    __attribute__((aligned(0x1000)));
static Thread g_thread;
static Mutex g_mutex;
static bool g_mutex_ready;
static bool g_running;
static bool g_thread_started;
static bool g_audio_open;

void lal_platform_shutdown(void);

static bool lal_switch_is_running(void) {
  bool running;

  mutexLock(&g_mutex);
  running = g_running;
  mutexUnlock(&g_mutex);
  return running;
}

static void lal_switch_fill(AudioOutBuffer *buffer) {
  int16_t source[LAL_SWITCH_INPUT_FRAMES * LAL_OUTPUT_CHANNELS];
  int16_t *output = (int16_t *) buffer->buffer;
  size_t output_index;

  mutexLock(&g_mutex);
  lal_mix_frames(source, LAL_SWITCH_INPUT_FRAMES);
  mutexUnlock(&g_mutex);

  /* audout is fixed at 48 kHz.  960 output frames consume exactly 882
   * 44.1-kHz mixer frames, avoiding cumulative timing drift. */
  for (output_index = 0;
       output_index < LAL_SWITCH_OUTPUT_FRAMES;
       ++output_index) {
    const size_t source_index = output_index * 147 / 160;
    output[output_index * LAL_OUTPUT_CHANNELS] =
      source[source_index * LAL_OUTPUT_CHANNELS];
    output[output_index * LAL_OUTPUT_CHANNELS + 1] =
      source[source_index * LAL_OUTPUT_CHANNELS + 1];
  }
  buffer->data_size =
    LAL_SWITCH_OUTPUT_FRAMES * LAL_OUTPUT_CHANNELS * sizeof(*output);
}

static void lal_switch_thread(void *unused) {
  (void) unused;
  while (lal_switch_is_running()) {
    AudioOutBuffer *released = NULL;
    u32 released_count = 0;
    const Result result = audoutWaitPlayFinish(
      &released, &released_count, UINT64_MAX);
    if (R_FAILED(result) || !lal_switch_is_running()) {
      break;
    }
    while (released) {
      AudioOutBuffer *const next = released->next;
      lal_switch_fill(released);
      if (R_FAILED(audoutAppendAudioOutBuffer(released))) {
        mutexLock(&g_mutex);
        g_running = false;
        mutexUnlock(&g_mutex);
        return;
      }
      released = next;
    }
  }
}

bool lal_platform_init(void) {
  int index;

  mutexInit(&g_mutex);
  g_mutex_ready = true;
  if (R_FAILED(audoutInitialize())) {
    lal_set_error("Could not initialize Switch audio output.");
    g_mutex_ready = false;
    return false;
  }
  if (R_FAILED(audoutStartAudioOut())) {
    lal_set_error("Could not start Switch audio output.");
    audoutExit();
    g_mutex_ready = false;
    return false;
  }
  g_audio_open = true;

  memset(g_buffers, 0, sizeof(g_buffers));
  for (index = 0; index < LAL_SWITCH_BUFFER_COUNT; ++index) {
    g_buffers[index].buffer = g_samples[index];
    g_buffers[index].buffer_size = sizeof(g_samples[index]);
    lal_switch_fill(&g_buffers[index]);
    if (R_FAILED(audoutAppendAudioOutBuffer(&g_buffers[index]))) {
      lal_set_error("Could not queue a Switch audio buffer.");
      lal_platform_shutdown();
      return false;
    }
  }

  mutexLock(&g_mutex);
  g_running = true;
  mutexUnlock(&g_mutex);
  if (R_FAILED(threadCreate(
          &g_thread,
          lal_switch_thread,
          NULL,
          NULL,
          0x4000,
          0x2B,
          -2))) {
    lal_set_error("Could not create the Switch audio thread.");
    lal_platform_shutdown();
    return false;
  }
  if (R_FAILED(threadStart(&g_thread))) {
    lal_set_error("Could not start the Switch audio thread.");
    threadClose(&g_thread);
    lal_platform_shutdown();
    return false;
  }
  g_thread_started = true;
  return true;
}

void lal_platform_shutdown(void) {
  if (g_mutex_ready) {
    mutexLock(&g_mutex);
    g_running = false;
    mutexUnlock(&g_mutex);
  }
  if (g_audio_open) {
    audoutStopAudioOut();
  }
  if (g_thread_started) {
    threadWaitForExit(&g_thread);
    threadClose(&g_thread);
    g_thread_started = false;
  }
  if (g_audio_open) {
    audoutExit();
    g_audio_open = false;
  }
  g_mutex_ready = false;
}

void lal_platform_lock(void) {
  if (g_mutex_ready) {
    mutexLock(&g_mutex);
  }
}

void lal_platform_unlock(void) {
  if (g_mutex_ready) {
    mutexUnlock(&g_mutex);
  }
}
