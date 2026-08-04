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

#include <aaudio/AAudio.h>

#include <pthread.h>

#define TAL_ANDROID_OUTPUT_SAMPLE_RATE 44100
#define TAL_ANDROID_OUTPUT_CHANNELS 2
#define TAL_ANDROID_CALLBACK_FRAMES 256u
#define TAL_ANDROID_MAX_INPUT_FRAMES 256u

typedef struct {
  Tal *tal;
  AAudioStream *stream;
  pthread_mutex_t mutex;
  uint32_t out_rate;
  bool mutex_ready;
  bool running;
  int16_t input[TAL_ANDROID_MAX_INPUT_FRAMES];
} TalAndroid;

size_t tal_backend_memory_alignment(void) {
  return _Alignof(TalAndroid);
}

size_t tal_backend_memory_required(const TalConfig *config) {
  (void) config;
  return sizeof(TalAndroid);
}

void tal_backend_lock(Tal *tal) {
  TalAndroid *android = tal ? (TalAndroid *) tal->backend : NULL;
  if (android && android->mutex_ready) {
    pthread_mutex_lock(&android->mutex);
  }
}

void tal_backend_unlock(Tal *tal) {
  TalAndroid *android = tal ? (TalAndroid *) tal->backend : NULL;
  if (android && android->mutex_ready) {
    pthread_mutex_unlock(&android->mutex);
  }
}

static aaudio_data_callback_result_t tal_android_callback(
    AAudioStream *stream, void *user_data, void *audio_data,
    int32_t frame_count) {
  TalAndroid *android = (TalAndroid *) user_data;
  int16_t *output = (int16_t *) audio_data;
  const uint32_t frames = (uint32_t) frame_count;
  const uint32_t input_rate = android->tal->config.sample_rate;
  uint32_t input_frames;
  uint32_t frame;
  (void) stream;

  input_frames = (uint32_t) ((uint64_t) frames * input_rate / android->out_rate);
  if (input_frames == 0u) {
    input_frames = 1u;
  }
  if (input_frames > TAL_ANDROID_MAX_INPUT_FRAMES) {
    input_frames = TAL_ANDROID_MAX_INPUT_FRAMES;
  }

  pthread_mutex_lock(&android->mutex);
  if (!android->running ||
      tal_internal_render(android->tal, android->input, input_frames) !=
        TAL_RESULT_OK) {
    pthread_mutex_unlock(&android->mutex);
    tal_internal_zero(
      output,
      (size_t) frames * TAL_ANDROID_OUTPUT_CHANNELS * sizeof(int16_t));
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
  }
  pthread_mutex_unlock(&android->mutex);

  for (frame = 0u; frame < frames; ++frame) {
    uint32_t index =
      (uint32_t) ((uint64_t) frame * input_rate / android->out_rate);
    if (index >= input_frames) {
      index = input_frames - 1u;
    }
    output[frame * TAL_ANDROID_OUTPUT_CHANNELS] = android->input[index];
    output[frame * TAL_ANDROID_OUTPUT_CHANNELS + 1u] = android->input[index];
  }
  return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

TalResult tal_backend_init(
    Tal *tal, void *memory, size_t memory_size, const TalConfig *config) {
  TalAndroid *android;
  AAudioStreamBuilder *builder = NULL;
  aaudio_result_t result;
  if (!tal || !memory || !config || memory_size < sizeof(TalAndroid)) {
    return TAL_RESULT_INVALID_ARGUMENT;
  }
  if (config->channels != 1u) {
    return TAL_RESULT_BACKEND_UNAVAILABLE;
  }

  android = (TalAndroid *) memory;
  android->tal = tal;
  android->stream = NULL;
  android->running = false;
  android->mutex_ready = false;
  android->out_rate = TAL_ANDROID_OUTPUT_SAMPLE_RATE;

  if (pthread_mutex_init(&android->mutex, NULL) != 0) {
    return TAL_RESULT_BACKEND_FAILURE;
  }
  android->mutex_ready = true;

  if (AAudio_createStreamBuilder(&builder) != AAUDIO_OK) {
    tal_backend_shutdown(tal);
    return TAL_RESULT_BACKEND_UNAVAILABLE;
  }
  AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
  AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
  AAudioStreamBuilder_setChannelCount(builder, TAL_ANDROID_OUTPUT_CHANNELS);
  AAudioStreamBuilder_setSampleRate(builder, TAL_ANDROID_OUTPUT_SAMPLE_RATE);
  AAudioStreamBuilder_setPerformanceMode(
    builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
  AAudioStreamBuilder_setFramesPerDataCallback(
    builder, TAL_ANDROID_CALLBACK_FRAMES);
  AAudioStreamBuilder_setDataCallback(builder, tal_android_callback, android);
  result = AAudioStreamBuilder_openStream(builder, &android->stream);
  AAudioStreamBuilder_delete(builder);
  if (result != AAUDIO_OK || !android->stream) {
    tal_backend_shutdown(tal);
    return TAL_RESULT_BACKEND_FAILURE;
  }

  if (AAudioStream_getChannelCount(android->stream) !=
        TAL_ANDROID_OUTPUT_CHANNELS ||
      AAudioStream_getFormat(android->stream) != AAUDIO_FORMAT_PCM_I16) {
    tal_backend_shutdown(tal);
    return TAL_RESULT_BACKEND_UNAVAILABLE;
  }
  android->out_rate = (uint32_t) AAudioStream_getSampleRate(android->stream);
  if (android->out_rate == 0u) {
    android->out_rate = TAL_ANDROID_OUTPUT_SAMPLE_RATE;
  }

  android->running = true;
  result = AAudioStream_requestStart(android->stream);
  if (result != AAUDIO_OK) {
    tal_backend_shutdown(tal);
    return TAL_RESULT_BACKEND_FAILURE;
  }
  return TAL_RESULT_OK;
}

void tal_backend_shutdown(Tal *tal) {
  TalAndroid *android = tal ? (TalAndroid *) tal->backend : NULL;
  if (!android) {
    return;
  }
  if (android->mutex_ready) {
    pthread_mutex_lock(&android->mutex);
    android->running = false;
    pthread_mutex_unlock(&android->mutex);
  }
  if (android->stream) {
    AAudioStream_requestStop(android->stream);
    AAudioStream_close(android->stream);
    android->stream = NULL;
  }
  if (android->mutex_ready) {
    pthread_mutex_destroy(&android->mutex);
    android->mutex_ready = false;
  }
}

TalResult tal_backend_update(Tal *tal) {
  (void) tal;
  return TAL_RESULT_OK;
}
