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

#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>
#include <pthread.h>

typedef struct {
  Tal *tal;
  AudioComponentInstance unit;
  pthread_mutex_t mutex;
  bool mutex_ready;
  bool running;
} TalCoreAudio;

size_t tal_backend_memory_alignment(void) {
  return _Alignof(TalCoreAudio);
}

size_t tal_backend_memory_required(const TalConfig *config) {
  (void) config;
  return sizeof(TalCoreAudio);
}

void tal_backend_lock(Tal *tal) {
  TalCoreAudio *core = tal ? (TalCoreAudio *) tal->backend : NULL;
  if (core && core->mutex_ready) pthread_mutex_lock(&core->mutex);
}

void tal_backend_unlock(Tal *tal) {
  TalCoreAudio *core = tal ? (TalCoreAudio *) tal->backend : NULL;
  if (core && core->mutex_ready) pthread_mutex_unlock(&core->mutex);
}

static void tal_coreaudio_zero(AudioBufferList *buffers) {
  UInt32 buffer_index;
  for (buffer_index = 0u;
       buffer_index < buffers->mNumberBuffers; ++buffer_index) {
    AudioBuffer *buffer = &buffers->mBuffers[buffer_index];
    uint8_t *bytes = (uint8_t *) buffer->mData;
    UInt32 index;
    for (index = 0u; bytes && index < buffer->mDataByteSize; ++index)
      bytes[index] = 0u;
  }
}

static OSStatus tal_coreaudio_render(
    void *user_data, AudioUnitRenderActionFlags *flags,
    const AudioTimeStamp *timestamp, UInt32 bus_number,
    UInt32 frame_count, AudioBufferList *buffers) {
  TalCoreAudio *core = (TalCoreAudio *) user_data;
  uint32_t offset = 0u;
  int16_t *output;
  size_t required_bytes;
  (void) flags;
  (void) timestamp;
  (void) bus_number;
  if (!core || !core->tal || !buffers) return noErr;
  tal_backend_lock(core->tal);
  required_bytes = (size_t) frame_count * core->tal->config.channels *
    sizeof(int16_t);
  if (!core->running || buffers->mNumberBuffers != 1u ||
      !buffers->mBuffers[0].mData ||
      buffers->mBuffers[0].mDataByteSize < required_bytes) {
    tal_coreaudio_zero(buffers);
    tal_backend_unlock(core->tal);
    return noErr;
  }
  output = (int16_t *) buffers->mBuffers[0].mData;
  while (offset < frame_count) {
    uint32_t block = frame_count - offset;
    if (block > core->tal->config.mix_block_frames)
      block = core->tal->config.mix_block_frames;
    if (tal_internal_render(
          core->tal,
          output + (size_t) offset * core->tal->config.channels,
          block) != TAL_RESULT_OK) {
      tal_coreaudio_zero(buffers);
      break;
    }
    offset += block;
  }
  tal_backend_unlock(core->tal);
  return noErr;
}

TalResult tal_backend_init(
    Tal *tal, void *memory, size_t memory_size, const TalConfig *config) {
  TalCoreAudio *core;
  AudioComponentDescription description;
  AudioComponent component;
  AudioStreamBasicDescription format;
  AURenderCallbackStruct callback;
  OSStatus status;
  if (!tal || !memory || memory_size < sizeof(TalCoreAudio) || !config)
    return TAL_RESULT_INVALID_ARGUMENT;
  core = (TalCoreAudio *) memory;
  core->tal = tal;
  if (pthread_mutex_init(&core->mutex, NULL) != 0)
    return TAL_RESULT_BACKEND_FAILURE;
  core->mutex_ready = true;
  tal_internal_zero(&description, sizeof(description));
  description.componentType = kAudioUnitType_Output;
  description.componentSubType = kAudioUnitSubType_DefaultOutput;
  description.componentManufacturer = kAudioUnitManufacturer_Apple;
  component = AudioComponentFindNext(NULL, &description);
  if (!component || AudioComponentInstanceNew(component, &core->unit) != noErr) {
    pthread_mutex_destroy(&core->mutex);
    core->mutex_ready = false;
    return TAL_RESULT_BACKEND_UNAVAILABLE;
  }
  tal_internal_zero(&format, sizeof(format));
  format.mSampleRate = config->sample_rate;
  format.mFormatID = kAudioFormatLinearPCM;
  format.mFormatFlags =
    kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked |
    kAudioFormatFlagsNativeEndian;
  format.mBytesPerPacket = config->channels * sizeof(int16_t);
  format.mFramesPerPacket = 1u;
  format.mBytesPerFrame = format.mBytesPerPacket;
  format.mChannelsPerFrame = config->channels;
  format.mBitsPerChannel = 16u;
  status = AudioUnitSetProperty(
    core->unit, kAudioUnitProperty_StreamFormat,
    kAudioUnitScope_Input, 0u, &format, sizeof(format));
  if (status != noErr) {
    tal_backend_shutdown(tal);
    return TAL_RESULT_BACKEND_FAILURE;
  }
  callback.inputProc = tal_coreaudio_render;
  callback.inputProcRefCon = core;
  status = AudioUnitSetProperty(
    core->unit, kAudioUnitProperty_SetRenderCallback,
    kAudioUnitScope_Input, 0u, &callback, sizeof(callback));
  if (status != noErr || AudioUnitInitialize(core->unit) != noErr) {
    tal_backend_shutdown(tal);
    return TAL_RESULT_BACKEND_FAILURE;
  }
  tal_backend_lock(tal);
  core->running = true;
  tal_backend_unlock(tal);
  if (AudioOutputUnitStart(core->unit) != noErr) {
    tal_backend_shutdown(tal);
    return TAL_RESULT_BACKEND_FAILURE;
  }
  return TAL_RESULT_OK;
}

void tal_backend_shutdown(Tal *tal) {
  TalCoreAudio *core = tal ? (TalCoreAudio *) tal->backend : NULL;
  if (!core || !core->mutex_ready) return;
  tal_backend_lock(tal);
  core->running = false;
  tal_backend_unlock(tal);
  if (core->unit) {
    AudioOutputUnitStop(core->unit);
    AudioUnitUninitialize(core->unit);
    AudioComponentInstanceDispose(core->unit);
    core->unit = NULL;
  }
  pthread_mutex_destroy(&core->mutex);
  core->mutex_ready = false;
}

TalResult tal_backend_update(Tal *tal) {
  (void) tal;
  return TAL_RESULT_OK;
}
