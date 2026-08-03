/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of TAL.
 *
 * TAL is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * TAL is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with TAL. If not, see <https://www.gnu.org/licenses/>.
 */

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "tal_internal.h"

#include <windows.h>
#include <mmsystem.h>

enum { TAL_WINMM_BUFFER_COUNT = 3 };

typedef struct {
  Tal *tal;
  HWAVEOUT output;
  CRITICAL_SECTION mutex;
  WAVEHDR headers[TAL_WINMM_BUFFER_COUNT];
  int16_t *samples;
  bool mutex_ready;
  bool running;
} TalWinmm;

static size_t tal_winmm_samples_offset(void) {
  return tal_internal_align_up(sizeof(TalWinmm), _Alignof(int16_t));
}

size_t tal_backend_memory_alignment(void) {
  return _Alignof(TalWinmm);
}

size_t tal_backend_memory_required(const TalConfig *config) {
  size_t offset = tal_winmm_samples_offset();
  size_t sample_count;
  if (!config || config->channels == 0u ||
      config->mix_block_frames >
      SIZE_MAX / (size_t) config->channels / TAL_WINMM_BUFFER_COUNT) {
    return 0u;
  }
  if (config->mix_block_frames >
      UINT32_MAX / ((uint32_t) config->channels * sizeof(int16_t))) {
    return 0u;
  }
  sample_count = (size_t) config->mix_block_frames *
    (size_t) config->channels * TAL_WINMM_BUFFER_COUNT;
  if (sample_count > (SIZE_MAX - offset) / sizeof(int16_t)) return 0u;
  return offset + sample_count * sizeof(int16_t);
}

void tal_backend_lock(Tal *tal) {
  TalWinmm *winmm = tal ? (TalWinmm *) tal->backend : NULL;
  if (winmm && winmm->mutex_ready) EnterCriticalSection(&winmm->mutex);
}

void tal_backend_unlock(Tal *tal) {
  TalWinmm *winmm = tal ? (TalWinmm *) tal->backend : NULL;
  if (winmm && winmm->mutex_ready) LeaveCriticalSection(&winmm->mutex);
}

static void tal_winmm_zero_buffer(TalWinmm *winmm, WAVEHDR *header) {
  int16_t *samples = (int16_t *) header->lpData;
  size_t count = (size_t) winmm->tal->config.mix_block_frames *
    winmm->tal->config.channels;
  size_t index;
  for (index = 0u; index < count; ++index) samples[index] = 0;
}

static void tal_winmm_fill_locked(TalWinmm *winmm, WAVEHDR *header) {
  if (tal_internal_render(
        winmm->tal, (int16_t *) header->lpData,
        winmm->tal->config.mix_block_frames) != TAL_RESULT_OK) {
    tal_winmm_zero_buffer(winmm, header);
  }
}

static void CALLBACK tal_winmm_callback(
    HWAVEOUT output, UINT message, DWORD_PTR instance,
    DWORD_PTR parameter_one, DWORD_PTR parameter_two) {
  TalWinmm *winmm = (TalWinmm *) instance;
  WAVEHDR *header = (WAVEHDR *) parameter_one;
  (void) output;
  (void) parameter_two;
  if (!winmm || message != WOM_DONE) return;
  tal_backend_lock(winmm->tal);
  if (winmm->running) {
    tal_winmm_fill_locked(winmm, header);
    if (waveOutWrite(winmm->output, header, sizeof(*header)) !=
        MMSYSERR_NOERROR) {
      winmm->running = false;
    }
  }
  tal_backend_unlock(winmm->tal);
}

TalResult tal_backend_init(
    Tal *tal, void *memory, size_t memory_size, const TalConfig *config) {
  TalWinmm *winmm;
  WAVEFORMATEX format;
  MMRESULT result;
  unsigned index;
  if (!tal || !memory || !config ||
      memory_size < tal_backend_memory_required(config)) {
    return TAL_RESULT_INVALID_ARGUMENT;
  }
  winmm = (TalWinmm *) memory;
  winmm->tal = tal;
  winmm->samples = (int16_t *)
    ((uint8_t *) memory + tal_winmm_samples_offset());
  InitializeCriticalSection(&winmm->mutex);
  winmm->mutex_ready = true;
  tal_internal_zero(&format, sizeof(format));
  format.wFormatTag = WAVE_FORMAT_PCM;
  format.nChannels = config->channels;
  format.nSamplesPerSec = config->sample_rate;
  format.wBitsPerSample = 16u;
  format.nBlockAlign =
    (WORD) ((uint32_t) config->channels * sizeof(int16_t));
  format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
  result = waveOutOpen(
    &winmm->output, WAVE_MAPPER, &format,
    (DWORD_PTR) tal_winmm_callback, (DWORD_PTR) winmm, CALLBACK_FUNCTION);
  if (result != MMSYSERR_NOERROR) {
    DeleteCriticalSection(&winmm->mutex);
    winmm->mutex_ready = false;
    return TAL_RESULT_BACKEND_UNAVAILABLE;
  }
  for (index = 0u; index < TAL_WINMM_BUFFER_COUNT; ++index) {
    WAVEHDR *header = &winmm->headers[index];
    header->lpData = (LPSTR) (winmm->samples +
      (size_t) index * config->mix_block_frames * config->channels);
    header->dwBufferLength =
      config->mix_block_frames * config->channels * sizeof(int16_t);
    result = waveOutPrepareHeader(winmm->output, header, sizeof(*header));
    if (result != MMSYSERR_NOERROR) {
      unsigned prepared;
      for (prepared = 0u; prepared < index; ++prepared)
        waveOutUnprepareHeader(
          winmm->output, &winmm->headers[prepared],
          sizeof(winmm->headers[prepared]));
      waveOutClose(winmm->output);
      winmm->output = NULL;
      DeleteCriticalSection(&winmm->mutex);
      winmm->mutex_ready = false;
      return TAL_RESULT_BACKEND_FAILURE;
    }
  }
  tal_backend_lock(tal);
  winmm->running = true;
  for (index = 0u; index < TAL_WINMM_BUFFER_COUNT; ++index)
    tal_winmm_fill_locked(winmm, &winmm->headers[index]);
  tal_backend_unlock(tal);
  for (index = 0u; index < TAL_WINMM_BUFFER_COUNT; ++index) {
    result = waveOutWrite(
      winmm->output, &winmm->headers[index], sizeof(winmm->headers[index]));
    if (result != MMSYSERR_NOERROR) {
      tal_backend_shutdown(tal);
      return TAL_RESULT_BACKEND_FAILURE;
    }
  }
  return TAL_RESULT_OK;
}

void tal_backend_shutdown(Tal *tal) {
  TalWinmm *winmm = tal ? (TalWinmm *) tal->backend : NULL;
  unsigned index;
  if (!winmm || !winmm->mutex_ready) return;
  tal_backend_lock(tal);
  winmm->running = false;
  tal_backend_unlock(tal);
  if (winmm->output) {
    waveOutReset(winmm->output);
    for (index = 0u; index < TAL_WINMM_BUFFER_COUNT; ++index) {
      if ((winmm->headers[index].dwFlags & WHDR_PREPARED) != 0u)
        waveOutUnprepareHeader(
          winmm->output, &winmm->headers[index],
          sizeof(winmm->headers[index]));
    }
    waveOutClose(winmm->output);
    winmm->output = NULL;
  }
  DeleteCriticalSection(&winmm->mutex);
  winmm->mutex_ready = false;
}

TalResult tal_backend_update(Tal *tal) {
  (void) tal;
  return TAL_RESULT_OK;
}
