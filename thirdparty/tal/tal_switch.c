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

#include <switch.h>

#define TAL_SWITCH_BUFFER_COUNT 3u
#define TAL_SWITCH_OUTPUT_FRAMES 640u
#define TAL_SWITCH_OUTPUT_CHANNELS 2u
#define TAL_SWITCH_BUFFER_BYTES 0x1000u
#define TAL_SWITCH_MAX_INPUT_FRAMES 512u
#define TAL_SWITCH_THREAD_STACK 0x4000u
#define TAL_SWITCH_THREAD_PRIORITY 0x2B
#define TAL_SWITCH_THREAD_CORE (-2)

typedef struct {
  Tal *tal;
  AudioOutBuffer buffers[TAL_SWITCH_BUFFER_COUNT];
  int16_t *samples[TAL_SWITCH_BUFFER_COUNT];
  Thread thread;
  Mutex mutex;
  uint32_t out_rate;
  uint32_t input_frames;
  bool mutex_ready;
  bool audio_open;
  bool thread_started;
  bool running;
} TalSwitch;

static size_t tal_switch_header_size(void) {
  return (sizeof(TalSwitch) + (TAL_SWITCH_BUFFER_BYTES - 1u)) &
    ~(size_t) (TAL_SWITCH_BUFFER_BYTES - 1u);
}

size_t tal_backend_memory_alignment(void) {
  return TAL_SWITCH_BUFFER_BYTES;
}

size_t tal_backend_memory_required(const TalConfig *config) {
  (void) config;
  return tal_switch_header_size() +
    (size_t) TAL_SWITCH_BUFFER_COUNT * TAL_SWITCH_BUFFER_BYTES;
}

void tal_backend_lock(Tal *tal) {
  TalSwitch *sw = tal ? (TalSwitch *) tal->backend : NULL;
  if (sw && sw->mutex_ready) mutexLock(&sw->mutex);
}

void tal_backend_unlock(Tal *tal) {
  TalSwitch *sw = tal ? (TalSwitch *) tal->backend : NULL;
  if (sw && sw->mutex_ready) mutexUnlock(&sw->mutex);
}

static bool tal_switch_is_running(TalSwitch *sw) {
  bool running;
  mutexLock(&sw->mutex);
  running = sw->running;
  mutexUnlock(&sw->mutex);
  return running;
}

static void tal_switch_fill(TalSwitch *sw, AudioOutBuffer *buffer) {
  int16_t source[TAL_SWITCH_MAX_INPUT_FRAMES];
  int16_t *output = (int16_t *) buffer->buffer;
  const uint32_t sample_rate = sw->tal->config.sample_rate;
  uint32_t frame;

  mutexLock(&sw->mutex);
  if (tal_internal_render(sw->tal, source, sw->input_frames) != TAL_RESULT_OK) {
    tal_internal_zero(source, (size_t) sw->input_frames * sizeof(int16_t));
  }
  mutexUnlock(&sw->mutex);

  for (frame = 0u; frame < TAL_SWITCH_OUTPUT_FRAMES; ++frame) {
    uint32_t index =
      (uint32_t) ((uint64_t) frame * sample_rate / sw->out_rate);
    if (index >= sw->input_frames) index = sw->input_frames - 1u;
    output[frame * TAL_SWITCH_OUTPUT_CHANNELS] = source[index];
    output[frame * TAL_SWITCH_OUTPUT_CHANNELS + 1u] = source[index];
  }

  buffer->data_size =
    TAL_SWITCH_OUTPUT_FRAMES * TAL_SWITCH_OUTPUT_CHANNELS * sizeof(int16_t);
  armDCacheFlush(output, buffer->data_size);
}

static void tal_switch_thread(void *arg) {
  TalSwitch *sw = (TalSwitch *) arg;
  while (tal_switch_is_running(sw)) {
    AudioOutBuffer *released = NULL;
    uint32_t released_count = 0u;
    if (R_FAILED(audoutWaitPlayFinish(
          &released, &released_count, UINT64_MAX)) ||
        !tal_switch_is_running(sw)) {
      break;
    }
    while (released) {
      AudioOutBuffer *const next = released->next;
      tal_switch_fill(sw, released);
      if (R_FAILED(audoutAppendAudioOutBuffer(released))) {
        mutexLock(&sw->mutex);
        sw->running = false;
        mutexUnlock(&sw->mutex);
        return;
      }
      released = next;
    }
  }
}

TalResult tal_backend_init(
    Tal *tal, void *memory, size_t memory_size, const TalConfig *config) {
  TalSwitch *sw;
  size_t header;
  uint32_t index;
  if (!tal || !memory || !config ||
      memory_size < tal_backend_memory_required(config)) {
    return TAL_RESULT_INVALID_ARGUMENT;
  }

  if (config->channels != 1u) {
    return TAL_RESULT_BACKEND_UNAVAILABLE;
  }

  sw = (TalSwitch *) memory;
  sw->tal = tal;
  header = tal_switch_header_size();
  for (index = 0u; index < TAL_SWITCH_BUFFER_COUNT; ++index) {
    sw->samples[index] = (int16_t *)
      ((uint8_t *) memory + header + (size_t) index * TAL_SWITCH_BUFFER_BYTES);
  }

  mutexInit(&sw->mutex);
  sw->mutex_ready = true;

  if (R_FAILED(audoutInitialize())) {
    return TAL_RESULT_BACKEND_UNAVAILABLE;
  }
  if (R_FAILED(audoutStartAudioOut())) {
    audoutExit();
    return TAL_RESULT_BACKEND_FAILURE;
  }
  sw->audio_open = true;

  sw->out_rate = audoutGetSampleRate();
  if (sw->out_rate == 0u) sw->out_rate = 48000u;
  sw->input_frames = (uint32_t) (
    ((uint64_t) TAL_SWITCH_OUTPUT_FRAMES * config->sample_rate +
      sw->out_rate - 1u) / sw->out_rate);
  if (sw->input_frames == 0u) sw->input_frames = 1u;
  if (sw->input_frames > TAL_SWITCH_MAX_INPUT_FRAMES) {
    tal_backend_shutdown(tal);
    return TAL_RESULT_BACKEND_UNAVAILABLE;
  }

  tal_internal_zero(sw->buffers, sizeof(sw->buffers));
  for (index = 0u; index < TAL_SWITCH_BUFFER_COUNT; ++index) {
    sw->buffers[index].buffer = sw->samples[index];
    sw->buffers[index].buffer_size = TAL_SWITCH_BUFFER_BYTES;
    tal_switch_fill(sw, &sw->buffers[index]);
    if (R_FAILED(audoutAppendAudioOutBuffer(&sw->buffers[index]))) {
      tal_backend_shutdown(tal);
      return TAL_RESULT_BACKEND_FAILURE;
    }
  }

  mutexLock(&sw->mutex);
  sw->running = true;
  mutexUnlock(&sw->mutex);
  if (R_FAILED(threadCreate(
        &sw->thread, tal_switch_thread, sw, NULL,
        TAL_SWITCH_THREAD_STACK, TAL_SWITCH_THREAD_PRIORITY,
        TAL_SWITCH_THREAD_CORE)) ||
      R_FAILED(threadStart(&sw->thread))) {
    tal_backend_shutdown(tal);
    return TAL_RESULT_BACKEND_FAILURE;
  }
  sw->thread_started = true;
  return TAL_RESULT_OK;
}

void tal_backend_shutdown(Tal *tal) {
  TalSwitch *sw = tal ? (TalSwitch *) tal->backend : NULL;
  if (!sw) return;
  if (sw->mutex_ready) {
    mutexLock(&sw->mutex);
    sw->running = false;
    mutexUnlock(&sw->mutex);
  }
  
  if (sw->audio_open) {
    audoutStopAudioOut();
  }
  if (sw->thread_started) {
    threadWaitForExit(&sw->thread);
    threadClose(&sw->thread);
    sw->thread_started = false;
  }
  if (sw->audio_open) {
    audoutExit();
    sw->audio_open = false;
  }
  sw->mutex_ready = false;
}

TalResult tal_backend_update(Tal *tal) {
  (void) tal;
  return TAL_RESULT_OK;
}
