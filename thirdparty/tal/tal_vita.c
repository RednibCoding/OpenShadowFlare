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

#include "tal_internal.h"

#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>

#define TAL_VITA_OUT_RATE 48000
#define TAL_VITA_OUT_FRAMES 1024u
#define TAL_VITA_OUT_CHANNELS 2u
#define TAL_VITA_MAX_INPUT_FRAMES 256u

typedef struct {
  int16_t output[TAL_VITA_OUT_FRAMES * TAL_VITA_OUT_CHANNELS];
  int16_t input[TAL_VITA_MAX_INPUT_FRAMES];
  Tal *tal;
  SceUID mutex;
  SceUID thread;
  int port;
  uint32_t in_rate;
  uint32_t input_frames;
  bool running;
  bool mutex_ready;
  bool port_ready;
  bool thread_ready;
} TalVita;

size_t tal_backend_memory_alignment(void) {
  return 64u;
}

size_t tal_backend_memory_required(const TalConfig *config) {
  (void) config;
  return sizeof(TalVita);
}

void tal_backend_lock(Tal *tal) {
  TalVita *vita = tal ? (TalVita *) tal->backend : NULL;
  if (vita && vita->mutex_ready) {
    sceKernelLockMutex(vita->mutex, 1, NULL);
  }
}

void tal_backend_unlock(Tal *tal) {
  TalVita *vita = tal ? (TalVita *) tal->backend : NULL;
  if (vita && vita->mutex_ready) {
    sceKernelUnlockMutex(vita->mutex, 1);
  }
}

static bool tal_vita_running(TalVita *vita) {
  bool running;
  sceKernelLockMutex(vita->mutex, 1, NULL);
  running = vita->running;
  sceKernelUnlockMutex(vita->mutex, 1);
  return running;
}

static void tal_vita_fill(TalVita *vita) {
  uint32_t frame;
  sceKernelLockMutex(vita->mutex, 1, NULL);
  if (tal_internal_render(vita->tal, vita->input, vita->input_frames) !=
      TAL_RESULT_OK) {
    tal_internal_zero(vita->input, (size_t) vita->input_frames * sizeof(int16_t));
  }
  sceKernelUnlockMutex(vita->mutex, 1);

  for (frame = 0u; frame < TAL_VITA_OUT_FRAMES; ++frame) {
    uint32_t index =
      (uint32_t) ((uint64_t) frame * vita->in_rate / TAL_VITA_OUT_RATE);
    if (index >= vita->input_frames) {
      index = vita->input_frames - 1u;
    }
    vita->output[frame * TAL_VITA_OUT_CHANNELS] = vita->input[index];
    vita->output[frame * TAL_VITA_OUT_CHANNELS + 1u] = vita->input[index];
  }
}

static int tal_vita_thread(SceSize args, void *argp) {
  TalVita *vita = *(TalVita **) argp;
  (void) args;
  while (tal_vita_running(vita)) {
    tal_vita_fill(vita);
    if (sceAudioOutOutput(vita->port, vita->output) < 0) {
      sceKernelLockMutex(vita->mutex, 1, NULL);
      vita->running = false;
      sceKernelUnlockMutex(vita->mutex, 1);
      break;
    }
  }
  return 0;
}

TalResult tal_backend_init(
    Tal *tal, void *memory, size_t memory_size, const TalConfig *config) {
  TalVita *vita;
  TalVita *thread_arg;
  if (!tal || !memory || !config || memory_size < sizeof(TalVita)) {
    return TAL_RESULT_INVALID_ARGUMENT;
  }
  if (config->channels != 1u) {
    return TAL_RESULT_BACKEND_UNAVAILABLE;
  }
  vita = (TalVita *) memory;
  vita->tal = tal;
  vita->mutex = -1;
  vita->thread = -1;
  vita->port = -1;
  vita->in_rate = config->sample_rate ? config->sample_rate : 11025u;
  vita->input_frames = (uint32_t) (
    (uint64_t) TAL_VITA_OUT_FRAMES * vita->in_rate / TAL_VITA_OUT_RATE);
  if (vita->input_frames == 0u) {
    vita->input_frames = 1u;
  }
  if (vita->input_frames > TAL_VITA_MAX_INPUT_FRAMES) {
    vita->input_frames = TAL_VITA_MAX_INPUT_FRAMES;
  }
  vita->running = false;
  vita->mutex_ready = false;
  vita->port_ready = false;
  vita->thread_ready = false;

  vita->mutex = sceKernelCreateMutex("osf_audio", 0, 0, NULL);
  if (vita->mutex < 0) {
    return TAL_RESULT_BACKEND_FAILURE;
  }
  vita->mutex_ready = true;

  vita->port = sceAudioOutOpenPort(
    SCE_AUDIO_OUT_PORT_TYPE_MAIN, (int) TAL_VITA_OUT_FRAMES,
    TAL_VITA_OUT_RATE, SCE_AUDIO_OUT_MODE_STEREO);
  if (vita->port < 0) {
    tal_backend_shutdown(tal);
    return TAL_RESULT_BACKEND_UNAVAILABLE;
  }
  vita->port_ready = true;

  vita->running = true;
  vita->thread = sceKernelCreateThread(
    "osf_audio", tal_vita_thread, 0x10000100, 0x10000, 0,
    SCE_KERNEL_THREAD_CPU_AFFINITY_MASK_DEFAULT, NULL);
  if (vita->thread < 0) {
    tal_backend_shutdown(tal);
    return TAL_RESULT_BACKEND_FAILURE;
  }
  thread_arg = vita;
  if (sceKernelStartThread(
        vita->thread, (SceSize) sizeof(thread_arg), &thread_arg) < 0) {
    tal_backend_shutdown(tal);
    return TAL_RESULT_BACKEND_FAILURE;
  }
  vita->thread_ready = true;
  return TAL_RESULT_OK;
}

void tal_backend_shutdown(Tal *tal) {
  TalVita *vita = tal ? (TalVita *) tal->backend : NULL;
  if (!vita) {
    return;
  }
  if (vita->mutex_ready) {
    sceKernelLockMutex(vita->mutex, 1, NULL);
    vita->running = false;
    sceKernelUnlockMutex(vita->mutex, 1);
  }
  if (vita->thread >= 0) {
    if (vita->thread_ready) {
      sceKernelWaitThreadEnd(vita->thread, NULL, NULL);
    }
    sceKernelDeleteThread(vita->thread);
    vita->thread = -1;
    vita->thread_ready = false;
  }
  if (vita->port_ready) {
    sceAudioOutReleasePort(vita->port);
    vita->port = -1;
    vita->port_ready = false;
  }
  if (vita->mutex_ready) {
    sceKernelDeleteMutex(vita->mutex);
    vita->mutex = -1;
    vita->mutex_ready = false;
  }
}

TalResult tal_backend_update(Tal *tal) {
  (void) tal;
  return TAL_RESULT_OK;
}
