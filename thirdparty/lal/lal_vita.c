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

#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>

#include <stdbool.h>
#include <stdint.h>

static int16_t g_samples[LAL_BUFFER_FRAMES * LAL_OUTPUT_CHANNELS]
    __attribute__((aligned(64)));
static SceUID g_mutex = -1;
static SceUID g_thread = -1;
static int g_audio_port = -1;
static bool g_running;

void lal_platform_shutdown(void);

static bool lal_vita_is_running(void) {
  bool running = false;

  if (g_mutex >= 0) {
    sceKernelLockMutex(g_mutex, 1, NULL);
    running = g_running;
    sceKernelUnlockMutex(g_mutex, 1);
  }
  return running;
}

static void lal_vita_fill(void) {
  if (g_mutex < 0) {
    return;
  }
  sceKernelLockMutex(g_mutex, 1, NULL);
  lal_mix_frames(g_samples, LAL_BUFFER_FRAMES);
  sceKernelUnlockMutex(g_mutex, 1);
}

static int lal_vita_audio_thread(SceSize arguments, void* argument) {
  (void) arguments;
  (void) argument;

  while (lal_vita_is_running()) {
    lal_vita_fill();
    if (sceAudioOutOutput(g_audio_port, g_samples) < 0) {
      if (g_mutex >= 0) {
        sceKernelLockMutex(g_mutex, 1, NULL);
        g_running = false;
        sceKernelUnlockMutex(g_mutex, 1);
      }
      break;
    }
  }
  return 0;
}

bool lal_platform_init(void) {
  g_mutex = sceKernelCreateMutex("osf_audio", 0, 0, NULL);
  if (g_mutex < 0) {
    lal_set_error("Could not create the Vita audio mutex.");
    return false;
  }

  g_audio_port = sceAudioOutOpenPort(
      SCE_AUDIO_OUT_PORT_TYPE_BGM,
      LAL_BUFFER_FRAMES,
      LAL_OUTPUT_SAMPLE_RATE,
      SCE_AUDIO_OUT_MODE_STEREO);
  if (g_audio_port < 0) {
    lal_set_error("Could not open the Vita audio output port.");
    lal_platform_shutdown();
    return false;
  }

  g_running = true;
  g_thread = sceKernelCreateThread(
      "osf_audio", lal_vita_audio_thread, 0x10000100, 0x10000,
      0, SCE_KERNEL_THREAD_CPU_AFFINITY_MASK_DEFAULT, NULL);
  if (g_thread < 0) {
    lal_set_error("Could not create the Vita audio thread.");
    lal_platform_shutdown();
    return false;
  }
  if (sceKernelStartThread(g_thread, 0, NULL) < 0) {
    lal_set_error("Could not start the Vita audio thread.");
    sceKernelDeleteThread(g_thread);
    g_thread = -1;
    lal_platform_shutdown();
    return false;
  }
  return true;
}

void lal_platform_shutdown(void) {
  if (g_mutex >= 0) {
    sceKernelLockMutex(g_mutex, 1, NULL);
    g_running = false;
    sceKernelUnlockMutex(g_mutex, 1);
  }
  if (g_thread >= 0) {
    sceKernelWaitThreadEnd(g_thread, NULL, NULL);
    sceKernelDeleteThread(g_thread);
    g_thread = -1;
  }
  if (g_audio_port >= 0) {
    sceAudioOutReleasePort(g_audio_port);
    g_audio_port = -1;
  }
  if (g_mutex >= 0) {
    sceKernelDeleteMutex(g_mutex);
    g_mutex = -1;
  }
  g_running = false;
}

void lal_platform_lock(void) {
  if (g_mutex >= 0) {
    sceKernelLockMutex(g_mutex, 1, NULL);
  }
}

void lal_platform_unlock(void) {
  if (g_mutex >= 0) {
    sceKernelUnlockMutex(g_mutex, 1);
  }
}
