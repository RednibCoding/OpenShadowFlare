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

#include <SDL.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static SDL_AudioDeviceID g_device;
static SDL_mutex *g_mutex;
static bool g_audio_initialized;

static void lal_wiiu_audio_callback(
    void *user_data, Uint8 *stream, int byte_count) {
  const size_t frame_count =
    (size_t) byte_count / (LAL_OUTPUT_CHANNELS * sizeof(int16_t));

  (void) user_data;
  SDL_LockMutex(g_mutex);
  lal_mix_frames((int16_t *) stream, frame_count);
  SDL_UnlockMutex(g_mutex);
}

bool lal_platform_init(void) {
  SDL_AudioSpec requested;

  if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
    lal_set_error("Could not initialize Wii U audio output.");
    return false;
  }
  g_audio_initialized = true;

  g_mutex = SDL_CreateMutex();
  if (!g_mutex) {
    lal_set_error("Could not create the Wii U audio mixer mutex.");
    lal_platform_shutdown();
    return false;
  }

  memset(&requested, 0, sizeof(requested));
  requested.freq = LAL_OUTPUT_SAMPLE_RATE;
  requested.format = AUDIO_S16SYS;
  requested.channels = LAL_OUTPUT_CHANNELS;
  requested.samples = LAL_BUFFER_FRAMES;
  requested.callback = lal_wiiu_audio_callback;

  g_device = SDL_OpenAudioDevice(
    NULL, 0, &requested, NULL, 0);
  if (g_device == 0) {
    lal_set_error("Could not open the Wii U audio playback device.");
    lal_platform_shutdown();
    return false;
  }

  SDL_PauseAudioDevice(g_device, 0);
  return true;
}

void lal_platform_shutdown(void) {
  if (g_device != 0) {
    SDL_CloseAudioDevice(g_device);
    g_device = 0;
  }
  if (g_mutex) {
    SDL_DestroyMutex(g_mutex);
    g_mutex = NULL;
  }
  if (g_audio_initialized) {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    g_audio_initialized = false;
  }
}

void lal_platform_lock(void) {
  if (g_mutex) {
    SDL_LockMutex(g_mutex);
  }
}

void lal_platform_unlock(void) {
  if (g_mutex) {
    SDL_UnlockMutex(g_mutex);
  }
}
