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

#include "lal_internal.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum {
  LAL_WAVEOUT_BUFFER_COUNT = 3
};

static HWAVEOUT g_wave_out;
static WAVEHDR g_headers[LAL_WAVEOUT_BUFFER_COUNT];
static int16_t g_samples[
  LAL_WAVEOUT_BUFFER_COUNT][LAL_BUFFER_FRAMES * LAL_OUTPUT_CHANNELS];
static CRITICAL_SECTION g_mutex;
static bool g_mutex_ready;
static bool g_running;

static void lal_waveout_fill(WAVEHDR *header) {
  EnterCriticalSection(&g_mutex);
  lal_mix_frames((int16_t *) header->lpData, LAL_BUFFER_FRAMES);
  LeaveCriticalSection(&g_mutex);
  header->dwBufferLength = sizeof(g_samples[0]);
}

static void CALLBACK lal_waveout_callback(
    HWAVEOUT wave_out,
    UINT message,
    DWORD_PTR instance,
    DWORD_PTR parameter_one,
    DWORD_PTR parameter_two) {
  WAVEHDR *header;
  bool running;

  (void) wave_out;
  (void) instance;
  (void) parameter_two;
  if (message != WOM_DONE) {
    return;
  }

  EnterCriticalSection(&g_mutex);
  running = g_running;
  LeaveCriticalSection(&g_mutex);
  if (!running) {
    return;
  }

  header = (WAVEHDR *) parameter_one;
  lal_waveout_fill(header);
  waveOutWrite(g_wave_out, header, sizeof(*header));
}

bool lal_platform_init(void) {
  WAVEFORMATEX format;
  MMRESULT result;
  int index;

  InitializeCriticalSection(&g_mutex);
  g_mutex_ready = true;

  memset(&format, 0, sizeof(format));
  format.wFormatTag = WAVE_FORMAT_PCM;
  format.nChannels = LAL_OUTPUT_CHANNELS;
  format.nSamplesPerSec = LAL_OUTPUT_SAMPLE_RATE;
  format.wBitsPerSample = 16;
  format.nBlockAlign =
    (WORD) (format.nChannels * format.wBitsPerSample / 8);
  format.nAvgBytesPerSec =
    format.nSamplesPerSec * format.nBlockAlign;

  result = waveOutOpen(
    &g_wave_out,
    WAVE_MAPPER,
    &format,
    (DWORD_PTR) lal_waveout_callback,
    0,
    CALLBACK_FUNCTION);
  if (result != MMSYSERR_NOERROR) {
    lal_set_error("Could not open the default waveOut playback device.");
    DeleteCriticalSection(&g_mutex);
    g_mutex_ready = false;
    return false;
  }

  memset(g_headers, 0, sizeof(g_headers));
  for (index = 0; index < LAL_WAVEOUT_BUFFER_COUNT; ++index) {
    g_headers[index].lpData = (LPSTR) g_samples[index];
    g_headers[index].dwBufferLength = sizeof(g_samples[index]);
    result = waveOutPrepareHeader(
      g_wave_out, &g_headers[index], sizeof(g_headers[index]));
    if (result != MMSYSERR_NOERROR) {
      int prepared;

      lal_set_error("Could not prepare a waveOut playback buffer.");
      for (prepared = 0; prepared < index; ++prepared) {
        waveOutUnprepareHeader(
          g_wave_out, &g_headers[prepared], sizeof(g_headers[prepared]));
      }
      waveOutClose(g_wave_out);
      g_wave_out = NULL;
      DeleteCriticalSection(&g_mutex);
      g_mutex_ready = false;
      return false;
    }
  }

  EnterCriticalSection(&g_mutex);
  g_running = true;
  LeaveCriticalSection(&g_mutex);
  for (index = 0; index < LAL_WAVEOUT_BUFFER_COUNT; ++index) {
    lal_waveout_fill(&g_headers[index]);
    result = waveOutWrite(
      g_wave_out, &g_headers[index], sizeof(g_headers[index]));
    if (result != MMSYSERR_NOERROR) {
      lal_set_error("Could not queue a waveOut playback buffer.");
      lal_platform_shutdown();
      return false;
    }
  }

  return true;
}

void lal_platform_shutdown(void) {
  int index;

  EnterCriticalSection(&g_mutex);
  g_running = false;
  LeaveCriticalSection(&g_mutex);
  waveOutReset(g_wave_out);

  for (index = 0; index < LAL_WAVEOUT_BUFFER_COUNT; ++index) {
    if ((g_headers[index].dwFlags & WHDR_PREPARED) != 0) {
      waveOutUnprepareHeader(
        g_wave_out, &g_headers[index], sizeof(g_headers[index]));
    }
  }
  waveOutClose(g_wave_out);
  g_wave_out = NULL;

  DeleteCriticalSection(&g_mutex);
  g_mutex_ready = false;
}

void lal_platform_lock(void) {
  if (g_mutex_ready) {
    EnterCriticalSection(&g_mutex);
  }
}

void lal_platform_unlock(void) {
  if (g_mutex_ready) {
    LeaveCriticalSection(&g_mutex);
  }
}
