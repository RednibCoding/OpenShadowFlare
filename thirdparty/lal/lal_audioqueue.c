/*
 * Copyright (C) 2026 Michael Binder
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

#include <AudioToolbox/AudioToolbox.h>
#include <pthread.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum {
  LAL_AUDIOQUEUE_BUFFER_COUNT = 3
};

static AudioQueueRef g_queue;
static AudioQueueBufferRef g_buffers[LAL_AUDIOQUEUE_BUFFER_COUNT];
static pthread_mutex_t g_mutex;
static bool g_mutex_ready;
static bool g_running;

static void lal_audioqueue_callback(
    void *user_data,
    AudioQueueRef queue,
    AudioQueueBufferRef buffer) {
  bool running;

  (void) user_data;
  pthread_mutex_lock(&g_mutex);
  running = g_running;
  if (running) {
    lal_mix_frames((int16_t *) buffer->mAudioData, LAL_BUFFER_FRAMES);
  }
  pthread_mutex_unlock(&g_mutex);

  if (!running) {
    return;
  }

  buffer->mAudioDataByteSize =
    LAL_BUFFER_FRAMES * LAL_OUTPUT_CHANNELS * sizeof(int16_t);
  AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
}

bool lal_platform_init(void) {
  AudioStreamBasicDescription format;
  OSStatus status;
  UInt32 buffer_size;
  int index;

  if (pthread_mutex_init(&g_mutex, NULL) != 0) {
    lal_set_error("Could not create the Audio Queue mixer mutex.");
    return false;
  }
  g_mutex_ready = true;

  memset(&format, 0, sizeof(format));
  format.mSampleRate = LAL_OUTPUT_SAMPLE_RATE;
  format.mFormatID = kAudioFormatLinearPCM;
  format.mFormatFlags =
    kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
  format.mBytesPerPacket =
    LAL_OUTPUT_CHANNELS * (UInt32) sizeof(int16_t);
  format.mFramesPerPacket = 1;
  format.mBytesPerFrame = format.mBytesPerPacket;
  format.mChannelsPerFrame = LAL_OUTPUT_CHANNELS;
  format.mBitsPerChannel = 16;

  status = AudioQueueNewOutput(
    &format,
    lal_audioqueue_callback,
    NULL,
    NULL,
    NULL,
    0,
    &g_queue);
  if (status != noErr) {
    lal_set_error("Could not open the default Audio Queue output device.");
    pthread_mutex_destroy(&g_mutex);
    g_mutex_ready = false;
    return false;
  }

  buffer_size =
    LAL_BUFFER_FRAMES * LAL_OUTPUT_CHANNELS * (UInt32) sizeof(int16_t);
  memset(g_buffers, 0, sizeof(g_buffers));
  for (index = 0; index < LAL_AUDIOQUEUE_BUFFER_COUNT; ++index) {
    status = AudioQueueAllocateBuffer(
      g_queue, buffer_size, &g_buffers[index]);
    if (status != noErr) {
      lal_set_error("Could not allocate an Audio Queue playback buffer.");
      AudioQueueDispose(g_queue, true);
      g_queue = NULL;
      pthread_mutex_destroy(&g_mutex);
      g_mutex_ready = false;
      return false;
    }
  }

  pthread_mutex_lock(&g_mutex);
  g_running = true;
  pthread_mutex_unlock(&g_mutex);
  for (index = 0; index < LAL_AUDIOQUEUE_BUFFER_COUNT; ++index) {
    lal_audioqueue_callback(NULL, g_queue, g_buffers[index]);
  }

  status = AudioQueueStart(g_queue, NULL);
  if (status != noErr) {
    lal_set_error("Could not start Audio Queue playback.");
    lal_platform_shutdown();
    return false;
  }

  return true;
}

void lal_platform_shutdown(void) {
  pthread_mutex_lock(&g_mutex);
  g_running = false;
  pthread_mutex_unlock(&g_mutex);

  AudioQueueStop(g_queue, true);
  AudioQueueDispose(g_queue, true);
  g_queue = NULL;

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
