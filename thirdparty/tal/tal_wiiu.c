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

#include <coreinit/cache.h>
#include <coreinit/mutex.h>
#include <sndcore2/core.h>
#include <sndcore2/device.h>
#include <sndcore2/voice.h>

#define TAL_WIIU_BUFFER_FRAMES 8192u
#define TAL_WIIU_LEAD_FRAMES 1024u
#define TAL_WIIU_VOICE_PRIORITY 31u
#define TAL_WIIU_VOLUME_FULL 0x8000u

typedef struct {
  Tal *tal;
  AXVoice *voice;
  int16_t *buffer;
  uint32_t buffer_frames;
  uint32_t write_offset;
  OSMutex mutex;
  bool mutex_ready;
  bool ax_owned;
  bool callback_ready;
  bool running;
} TalWiiU;

static TalWiiU *tal_wiiu_active;

static size_t tal_wiiu_header_size(void) {
  return (sizeof(TalWiiU) + 63u) & ~(size_t) 63u;
}

size_t tal_backend_memory_alignment(void) {
  return 64u;
}

size_t tal_backend_memory_required(const TalConfig *config) {
  (void) config;
  return tal_wiiu_header_size() +
    (size_t) TAL_WIIU_BUFFER_FRAMES * sizeof(int16_t);
}

void tal_backend_lock(Tal *tal) {
  TalWiiU *wiiu = tal ? (TalWiiU *) tal->backend : NULL;
  if (wiiu && wiiu->mutex_ready) OSLockMutex(&wiiu->mutex);
}

void tal_backend_unlock(Tal *tal) {
  TalWiiU *wiiu = tal ? (TalWiiU *) tal->backend : NULL;
  if (wiiu && wiiu->mutex_ready) OSUnlockMutex(&wiiu->mutex);
}

static void tal_wiiu_fill(TalWiiU *wiiu, uint32_t count) {
  const uint32_t block_max = wiiu->tal->config.mix_block_frames;
  while (count > 0u) {
    uint32_t chunk = count;
    const uint32_t until_wrap = wiiu->buffer_frames - wiiu->write_offset;
    if (chunk > until_wrap) chunk = until_wrap;
    if (chunk > block_max) chunk = block_max;
    if (tal_internal_render(
          wiiu->tal, wiiu->buffer + wiiu->write_offset, chunk) !=
        TAL_RESULT_OK) {
      tal_internal_zero(
        wiiu->buffer + wiiu->write_offset, (size_t) chunk * sizeof(int16_t));
    }
    DCFlushRange(
      wiiu->buffer + wiiu->write_offset,
      (uint32_t) ((size_t) chunk * sizeof(int16_t)));
    wiiu->write_offset += chunk;
    if (wiiu->write_offset >= wiiu->buffer_frames)
      wiiu->write_offset -= wiiu->buffer_frames;
    count -= chunk;
  }
}

static void tal_wiiu_frame_callback(void) {
  TalWiiU *wiiu = tal_wiiu_active;
  if (!wiiu || !wiiu->mutex_ready) return;
  OSLockMutex(&wiiu->mutex);
  if (wiiu->running && wiiu->voice) {
    const uint32_t play =
      AXGetVoiceCurrentOffsetEx(wiiu->voice, wiiu->buffer);
    if (play < wiiu->buffer_frames) {
      uint32_t target = play + TAL_WIIU_LEAD_FRAMES;
      uint32_t count;
      if (target >= wiiu->buffer_frames) target -= wiiu->buffer_frames;
      count = (target + wiiu->buffer_frames - wiiu->write_offset) %
        wiiu->buffer_frames;
      if (count > 0u) tal_wiiu_fill(wiiu, count);
    }
  }
  OSUnlockMutex(&wiiu->mutex);
}

static bool tal_wiiu_start_voice(TalWiiU *wiiu, const TalConfig *config) {
  AXVoiceOffsets offsets;
  AXVoiceVeData volume;
  AXVoiceDeviceMixData mix[6];
  wiiu->voice = AXAcquireVoice(TAL_WIIU_VOICE_PRIORITY, NULL, NULL);
  if (!wiiu->voice) return false;

  AXVoiceBegin(wiiu->voice);
  AXSetVoiceType(wiiu->voice, 0);

  tal_internal_zero(&volume, sizeof(volume));
  volume.volume = TAL_WIIU_VOLUME_FULL;
  AXSetVoiceVe(wiiu->voice, &volume);

  tal_internal_zero(mix, sizeof(mix));
  mix[0].bus[0].volume = TAL_WIIU_VOLUME_FULL;
  mix[1].bus[0].volume = TAL_WIIU_VOLUME_FULL;
  AXSetVoiceDeviceMix(wiiu->voice, AX_DEVICE_TYPE_TV, 0, mix);
  AXSetVoiceDeviceMix(wiiu->voice, AX_DEVICE_TYPE_DRC, 0, mix);

  AXSetVoiceSrcType(wiiu->voice, AX_VOICE_SRC_TYPE_LINEAR);
  AXSetVoiceSrcRatio(
    wiiu->voice,
    (float) config->sample_rate / (float) AXGetInputSamplesPerSec());

  tal_internal_zero(&offsets, sizeof(offsets));
  offsets.dataType = AX_VOICE_FORMAT_LPCM16;
  offsets.loopingEnabled = AX_VOICE_LOOP_ENABLED;
  offsets.loopOffset = 0u;
  offsets.endOffset = wiiu->buffer_frames - 1u;
  offsets.currentOffset = 0u;
  offsets.data = wiiu->buffer;
  AXSetVoiceOffsets(wiiu->voice, &offsets);

  AXSetVoiceState(wiiu->voice, AX_VOICE_STATE_PLAYING);
  AXVoiceEnd(wiiu->voice);
  return true;
}

TalResult tal_backend_init(
    Tal *tal, void *memory, size_t memory_size, const TalConfig *config) {
  TalWiiU *wiiu;
  if (!tal || !memory || !config ||
      memory_size < tal_backend_memory_required(config)) {
    return TAL_RESULT_INVALID_ARGUMENT;
  }

  if (config->channels != 1u) {
    return TAL_RESULT_BACKEND_UNAVAILABLE;
  }

  wiiu = (TalWiiU *) memory;
  wiiu->tal = tal;
  wiiu->buffer = (int16_t *) ((uint8_t *) memory + tal_wiiu_header_size());
  wiiu->buffer_frames = TAL_WIIU_BUFFER_FRAMES;
  wiiu->write_offset = 0u;
  OSInitMutex(&wiiu->mutex);
  wiiu->mutex_ready = true;

  DCFlushRange(
    wiiu->buffer,
    (uint32_t) ((size_t) wiiu->buffer_frames * sizeof(int16_t)));

  if (!AXIsInit()) {
    AXInitParams params;
    tal_internal_zero(&params, sizeof(params));
    params.renderer = AX_INIT_RENDERER_48KHZ;
    params.pipeline = AX_INIT_PIPELINE_SINGLE;
    AXInitWithParams(&params);
    wiiu->ax_owned = true;
  }
  if (!AXIsInit()) {
    return TAL_RESULT_BACKEND_UNAVAILABLE;
  }

  if (!tal_wiiu_start_voice(wiiu, config)) {
    tal_backend_shutdown(tal);
    return TAL_RESULT_BACKEND_FAILURE;
  }

  tal_wiiu_active = wiiu;
  wiiu->running = true;
  if (AXRegisterAppFrameCallback(tal_wiiu_frame_callback) == 0)
    wiiu->callback_ready = true;
  return TAL_RESULT_OK;
}

void tal_backend_shutdown(Tal *tal) {
  TalWiiU *wiiu = tal ? (TalWiiU *) tal->backend : NULL;
  if (!wiiu) return;
  if (wiiu->mutex_ready) {
    OSLockMutex(&wiiu->mutex);
    wiiu->running = false;
    OSUnlockMutex(&wiiu->mutex);
  }
  if (wiiu->callback_ready) {
    AXDeregisterAppFrameCallback(tal_wiiu_frame_callback);
    wiiu->callback_ready = false;
  }
  if (wiiu->voice) {
    AXVoiceBegin(wiiu->voice);
    AXSetVoiceState(wiiu->voice, AX_VOICE_STATE_STOPPED);
    AXVoiceEnd(wiiu->voice);
    AXFreeVoice(wiiu->voice);
    wiiu->voice = NULL;
  }
  if (wiiu->ax_owned) {
    AXQuit();
    wiiu->ax_owned = false;
  }
  tal_wiiu_active = NULL;
}

TalResult tal_backend_update(Tal *tal) {
  (void) tal;
  return TAL_RESULT_OK;
}
