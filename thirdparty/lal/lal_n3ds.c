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

#include <3ds.h>

#include <string.h>

enum { LAL_N3DS_BUFFER_COUNT = 3 };

static int16_t *g_samples[LAL_N3DS_BUFFER_COUNT];
static ndspWaveBuf g_wave_buffers[LAL_N3DS_BUFFER_COUNT];
static LightLock g_lock;
static bool g_initialized;

static void refill_buffer(int index) {
  ndspWaveBuf *wave_buffer = &g_wave_buffers[index];

  if (wave_buffer->status != NDSP_WBUF_FREE &&
      wave_buffer->status != NDSP_WBUF_DONE) {
    return;
  }
  LightLock_Lock(&g_lock);
  lal_mix_frames(g_samples[index], LAL_BUFFER_FRAMES);
  LightLock_Unlock(&g_lock);
  DSP_FlushDataCache(
    g_samples[index],
    LAL_BUFFER_FRAMES * LAL_OUTPUT_CHANNELS * sizeof(*g_samples[index]));
  memset(wave_buffer, 0, sizeof(*wave_buffer));
  wave_buffer->data_pcm16 = g_samples[index];
  wave_buffer->nsamples = LAL_BUFFER_FRAMES * LAL_OUTPUT_CHANNELS;
  ndspChnWaveBufAdd(0, wave_buffer);
}

static void ndsp_callback(void *unused) {
  int index;
  (void) unused;
  for (index = 0; index < LAL_N3DS_BUFFER_COUNT; ++index) {
    refill_buffer(index);
  }
}

bool lal_platform_init(void) {
  float mix[12] = {};
  int index;

  if (R_FAILED(ndspInit())) {
    lal_set_error("Could not initialize Nintendo 3DS audio.");
    return false;
  }
  LightLock_Init(&g_lock);
  for (index = 0; index < LAL_N3DS_BUFFER_COUNT; ++index) {
    g_samples[index] = linearAlloc(
      LAL_BUFFER_FRAMES * LAL_OUTPUT_CHANNELS * sizeof(*g_samples[index]));
    if (!g_samples[index]) {
      lal_set_error("Could not allocate Nintendo 3DS audio buffers.");
      lal_platform_shutdown();
      return false;
    }
  }

  ndspSetOutputMode(NDSP_OUTPUT_STEREO);
  ndspChnReset(0);
  ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
  ndspChnSetRate(0, LAL_OUTPUT_SAMPLE_RATE);
  ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
  mix[0] = 1.0f;
  mix[1] = 1.0f;
  ndspChnSetMix(0, mix);
  g_initialized = true;
  ndspSetCallback(ndsp_callback, NULL);
  ndsp_callback(NULL);
  return true;
}

void lal_platform_shutdown(void) {
  int index;

  if (!g_initialized) {
    for (index = 0; index < LAL_N3DS_BUFFER_COUNT; ++index) {
      linearFree(g_samples[index]);
      g_samples[index] = NULL;
    }
    return;
  }
  ndspSetCallback(NULL, NULL);
  ndspChnWaveBufClear(0);
  ndspChnReset(0);
  ndspExit();
  for (index = 0; index < LAL_N3DS_BUFFER_COUNT; ++index) {
    linearFree(g_samples[index]);
    g_samples[index] = NULL;
    memset(&g_wave_buffers[index], 0, sizeof(g_wave_buffers[index]));
  }
  g_initialized = false;
}

void lal_platform_lock(void) {
  if (g_initialized) {
    LightLock_Lock(&g_lock);
  }
}

void lal_platform_unlock(void) {
  if (g_initialized) {
    LightLock_Unlock(&g_lock);
  }
}
